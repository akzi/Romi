#include "romi.hpp"

namespace romi
{

	const char *io_engine_monitor = "inproc://io_engine.monitor";
	const char *cmd_queue_inproc = "inproc://cmd_queue.ipc";

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
		case romi::command::e_null:
			break;
		case romi::command::e_net_connect:
			delete net_connect_;
			break;
		case romi::command::e_send_msg:
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
		if (!zmq_connect(socket_, cmd_queue_inproc))
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
		if (!zmq_send(socket_, "K", 1, 0))
		{
			throw std::runtime_error(zmq_strerror(errno));
		}
	}

	//io_engine
	io_engine::io_engine()
	{
		init();
	}

	void io_engine::bind(int port)
	{
		socket_ = zmq_socket(zmq_ctx_, ZMQ_REP);
		if (!socket_)
			throw std::runtime_error(zmq_strerror(errno));
		std::string addr = "tcp://:" + std::to_string(port);
		auto rc = zmq_bind(socket_, addr.c_str());
		if (rc == -1)
		{
			zmq_close(socket_);
			throw std::runtime_error(zmq_strerror(errno));
		}

		rc = zmq_socket_monitor(socket_, io_engine_monitor , ZMQ_EVENT_ALL);
		if (rc == -1)
		{
			zmq_close(socket_);
			throw std::runtime_error(zmq_strerror(errno));
		}
		auto monitor_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
		rc = zmq_connect(socket_, io_engine_monitor);
		if (rc == -1)
		{
			zmq_close(socket_);
			zmq_close(monitor_);
			throw std::runtime_error(zmq_strerror(errno));
		}
	}

	void io_engine::bind_send_msg_to_actor(std::function<void(message_base::ptr&&)> handle)
	{
		send_msg_to_actor_ = handle;
	}

	void io_engine::bind_handle_msg(std::function<void(void*, std::size_t)> handle)
	{
		handle_msg_ = handle;
	}

	void io_engine::send_cmd(command &&msg_)
	{
		msg_queue_.push_back(std::move(msg_));
	}

	void io_engine::start()
	{
		thread_ = std::thread([this] {run();});
		thread_.detach();
	}

	void io_engine::stop()
	{
		is_stop_ = true;
	}

	void io_engine::run()
	{
		do
		{
			zmq_pollitem_t pollitem[3] = {
				{ socket_, 0, ZMQ_POLLIN, 0 },
				{ cmd_queue_inproc_, 0, ZMQ_POLLIN, 0 },
				{ monitor_, 0, ZMQ_POLLIN, 0}
			};

			int ret = zmq_poll(&pollitem[0], 3, -1);
			if (ret == 0)
				return;

			else if (ret < 0)
			{
				std::cout << zmq_strerror(errno) << std::endl;
				return;
			}
			else if (pollitem[0].revents | ZMQ_POLLIN)
			{
				handle_msg_event();
			}
			else if (pollitem[1].revents | ZMQ_POLLIN)
			{
				handle_cmd_queue_event();
			}
			else if (pollitem[2].revents | ZMQ_POLLIN)
			{
				handle_monitor_event();
			}

		} while (is_stop_ == false);
	}

	void io_engine::init()
	{
		zmq_ctx_ = zmq_init(1);
		cmd_queue_inproc_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
		if (!zmq_bind(cmd_queue_inproc_, io_engine_monitor))
			throw std::runtime_error(zmq_strerror(errno));
		msg_queue_.init(zmq_ctx_);
	}

	io_engine::socket io_engine::connect(std::string remote_addr)
	{
		auto socket_ = zmq_socket(zmq_ctx_, ZMQ_REQ);
		if (!socket_)
			throw std::runtime_error(zmq_strerror(errno));
		int rc = zmq_connect(socket_, remote_addr.c_str());
		if (rc != 0)
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
		addr from;
		from.set_engine_id(0);
		from.set_actor_id(0);
		sys::net_connect_notify notify;
		notify.set_connected(false);

		auto socket = connect(remote_addr);
		if (socket)
		{
			sockets_[engine_id] = socket;
			notify.set_connected(true);
		}
		*notify.mutable_net_connect() = *_msg.net_connect_;
		send_msg_to_actor_(make_message(from, to, notify));
	}

	void io_engine::do_send(command &_msg)
	{
		auto &message = _msg.send_msg_->message_;
		uint64_t engine_id = message->to().engine_id();
		const auto itr = sockets_.find(engine_id);
		if (itr == sockets_.end())
		{
			addr from;
			sys::net_not_engine_id not_engine_id;

			from.set_actor_id(0);
			from.set_engine_id(message->from().engine_id());
			*not_engine_id.mutable_addr() = message->to();
			send_msg_to_actor_(make_message(from, message->from(), not_engine_id));
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
				return do_connect(std::move(cmd));
			case command::e_send_msg:
				return do_send(std::move(cmd));
			default:
				break;
			}
		}
	}
	void io_engine::handle_monitor_event()
	{
		zmq_msg_t msg;
		zmq_msg_init(&msg);
		if (zmq_recvmsg(monitor_, &msg, 0) == -1)
		{
			std::cout << zmq_strerror(errno) << std::endl;
			return;
		}

		uint8_t *data = (uint8_t *)zmq_msg_data(&msg);
		uint16_t event = *(uint16_t *)(data);
		uint32_t value = *(uint32_t *)(data + 2);

		zmq_msg_init(&msg);
		if (zmq_msg_recv(&msg, monitor_, 0) == -1)
		{
			std::cout << zmq_strerror(errno) << std::endl;
			return;
		}
		std::string addr;
		data = (uint8_t*)zmq_msg_data(&msg);
		size_t size = zmq_msg_size(&msg);
		addr.append((char*)data, size);

		std::cout << addr.c_str() << " event:" << event 
			<< " value:" << value << std::endl;
	}

	void io_engine::handle_cmd_queue_event()
	{
		zmq_msg_t msg;
		zmq_msg_init(&msg);
		int rc = zmq_recvmsg(cmd_queue_inproc_, &msg, 0);
		assert(rc != -1);
		assert(*(char*)zmq_msg_data(&msg) == 'K');
		zmq_msg_close(&msg);
		process_msg();
	}

	void io_engine::handle_msg_event()
	{
		zmq_msg_t msg;
		zmq_msg_init(&msg);

		if (zmq_recvmsg(socket_, &msg, 0) != -1)
		{
			try
			{
				handle_msg_(zmq_msg_data(&msg), zmq_msg_size(&msg));
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
		}
		else
		{
			std::cout << zmq_strerror(errno) << std::endl;
		}
		zmq_msg_close(&msg);
	}

}

