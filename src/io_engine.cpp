#include "romi.hpp"

namespace romi
{

const char *inproc_addr = "inproc://cmd_queue.ipc";

namespace net
{
	command & command::operator=(command &&other)
	{
		if (&other == this)
			return *this;
		this->~command();
		memcpy(this, &other, sizeof(command));
		memset(&other, 0, sizeof(command));
		return *this;
	}

	command::command(command &&other)
	{
		*this = std::move(other);
	}

	command::command()
	{
		memset(this, 0, sizeof(command));
	}

	command::~command()
	{
		switch (type_)
		{
		case command::e_null:
			break;
		case command::e_net_connect:
			delete net_connect_;
			break;
		case command::e_send_msg:
			delete send_msg_;
			break;
		default:
			break;
		}
	}

	//msg_queue
	cmd_queue::cmd_queue()
	{

	}

	void cmd_queue::init(void *zmq_ctx_)
	{
		socket_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
		if (zmq_connect(socket_, inproc_addr) == -1)
			throw std::runtime_error(zmq_strerror(errno));
		zmq_msg_t msg;
		zmq_msg_init_size(&msg, 1);
		*reinterpret_cast<char*>(zmq_msg_data(&msg)) = 'K';
	}

	void cmd_queue::push_back(command &&_msg)
	{
		if (msg_queue_.push(std::forward<command>(_msg)) == 1)
		{
			notify();
		}
	}

	bool cmd_queue::pop(command &_msg)
	{
		return msg_queue_.pop(_msg);
	}

	void cmd_queue::notify()
	{
		std::lock_guard<std::mutex> locker(socket_mutex_);
		if (zmq_send(socket_, "K", 1, 0) == -1)
		{
			throw std::runtime_error(zmq_strerror(errno));
		}
	}

	//io_engine
	io_engine::io_engine()
	{
		init();
	}

	void io_engine::bind(const std::string &addr)
	{
		bind_addr_ = addr;
	}

	void io_engine::bind_send_msg(
		std::function<void(message_base::ptr&&)> handle)
	{
		send_msg_ = handle;
	}

	void io_engine::bind_handle_net_msg(
		std::function<void(void*, std::size_t)> handle)
	{
		handle_msg_ = handle;
	}

	void io_engine::send_cmd(command &&msg_)
	{
		msg_queue_.push_back(std::move(msg_));
	}

	void io_engine::start()
	{
		start_subscriber();
		start_publisher();
	}

	void io_engine::stop()
	{
		is_stop_ = true;
		recevicer_.join();
		sender_.join();
	}
	void io_engine::start_subscriber()
	{
		socket_ = zmq_socket(zmq_ctx_, ZMQ_REP);
		if (!socket_)
			throw std::runtime_error(zmq_strerror(errno));

		auto rc = zmq_bind(socket_, bind_addr_.c_str());
		if (rc == -1)
		{
			zmq_close(socket_);
			throw std::runtime_error(zmq_strerror(errno));
		}

		int timeout = 100;
		rc = zmq_setsockopt(socket_, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
		if (rc == -1)
		{
			zmq_close(socket_);
			throw std::runtime_error("zmq_setsockopt failed");
		}

		zmq_msg_t msg;
		zmq_msg_init(&msg);
		while (is_stop_ == false)
		{
			if (zmq_recvmsg(socket_, &msg, 0) == -1)
			{
				if (errno == EAGAIN)
					continue;
				std::cout << zmq_strerror(errno) << std::endl;
			}
			try
			{
				handle_msg_(zmq_msg_data(&msg), zmq_msg_size(&msg));
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
		}
		zmq_msg_close(&msg);
	}


	void io_engine::start_publisher()
	{
	
		int timeout = 100;
		int rc = zmq_setsockopt(socket_, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
		if (rc == -1)
			throw std::runtime_error("zmq_setsockopt failed");

		sender_ = std::thread([this] {
			
			while (is_stop_ == false)
			{
				zmq_msg_t msg;
				zmq_msg_init(&msg);

				int rc = zmq_recvmsg(cmd_socket_, &msg, 0);
				if (rc == -1)
				{
					if(errno == EAGAIN)
						continue;
					throw std::runtime_error(zmq_strerror(errno));
				}
				zmq_msg_close(&msg);
				process_msg();
			}
		});
		
	}

	void io_engine::init()
	{
		zmq_ctx_ = zmq_init(1);
		cmd_socket_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
		if (zmq_bind(cmd_socket_, inproc_addr ) == -1)
			throw std::runtime_error(zmq_strerror(errno));
		msg_queue_.init(zmq_ctx_);
	}

	io_engine::socket io_engine::connect(std::string remote_addr)
	{
		auto socket_ = zmq_socket(zmq_ctx_, ZMQ_REQ);
		if (!socket_)
			throw std::runtime_error(zmq_strerror(errno));
		if (zmq_connect(socket_, remote_addr.c_str()) == -1)
		{
			zmq_close(socket_);
			return nullptr;
		}
		return socket_;
	}

	void io_engine::do_connect(command &_msg)
	{
		auto engine_id = _msg.net_connect_->engine_id();
		auto remote_addr = _msg.net_connect_->remote_addr();
		addr to = _msg.net_connect_->from();

		sys::net_connect_notify notify;
		notify.set_connected(false);

		auto socket = connect(remote_addr);
		if (socket)
		{
			sockets_[engine_id] = socket;
			notify.set_connected(true);
		}
		notify.mutable_net_connect()->CopyFrom(*_msg.net_connect_);
		send_msg_(make_message(to, to, notify));
	}

	void io_engine::do_send(command &_msg)
	{
		auto &message = _msg.send_msg_->message_;
		uint64_t engine_id = message->to().engine_id();
		const auto itr = sockets_.find(engine_id);
		if (itr == sockets_.end())
		{
			sys::net_not_engine_id not_engine_id;

			not_engine_id.mutable_addr()->CopyFrom(message->to());
			send_msg_(make_message(message->from(), message->from(), not_engine_id));
			return;
		}
		auto buffer = message->serialize_as_string();
		int rc = zmq_send(itr->second, buffer.data(), buffer.size(), 0);
		if (rc == -1)
			std::cout << zmq_strerror(errno) << std::endl;
	}

	void io_engine::process_msg()
	{
		while (true)
		{
			command cmd;
			if (!msg_queue_.pop(cmd))
				return;
			switch (cmd.type_)
			{
			case command::e_net_connect:
				do_connect(std::move(cmd));
				break;
			case command::e_send_msg:
				do_send(std::move(cmd));
				break;
			default:
				break;
			}
		}
	}
}
}

