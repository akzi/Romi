#include "romi.hpp"

namespace romi
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
	msg_queue::msg_queue()
	{

	}

	void msg_queue::init(void *zmq_ctx_)
	{
		socket_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
		if (!zmq_connect(socket_, "inproc://msg_queue.ipc"))
			throw std::runtime_error(zmq_strerror(errno));
		zmq_msg_t msg;
		zmq_msg_init_size(&msg, 1);
		*reinterpret_cast<char*>(zmq_msg_data(&msg)) = 'K';
	}

	void msg_queue::push_back(command &&_msg)
	{
		if (msg_queue_.push(std::forward<command>(_msg)) == 1)
		{
			notify();
		}
	}

	bool msg_queue::pop_msg(command &_msg)
	{
		return msg_queue_.pop(_msg);
	}

	void msg_queue::notify()
	{
		std::lock_guard<std::mutex> locker(socket_mutex_);
		if (!zmq_send(socket_, "K", 1, 0))
		{
			throw std::runtime_error(zmq_strerror(errno));
		}
	}

	net::net()
	{

	}

	void net::bind(int port)
	{
		socket_ = zmq_socket(zmq_ctx_, ZMQ_REP);
		if (!socket_)
			throw std::runtime_error(zmq_strerror(errno));
		std::string addr = "tcp://:" + std::to_string(port);
		if (!zmq_bind(socket_, addr.c_str()))
		{
			zmq_close(socket_);
			throw std::runtime_error(zmq_strerror(errno));
		}
	}

	void net::bind_send_msg_to_actor(std::function<void(message_base::ptr&&)> handle)
	{
		send_msg_to_actor_ = handle;
	}

	void net::bind_handle_msg(std::function<void(void*, std::size_t)> handle)
	{
		handle_msg_ = handle;
	}

	void net::send_msg(command &&msg_)
	{
		msg_queue_.push_back(std::move(msg_));
	}

	void net::start()
	{
		thread_ = std::thread([this] {

			do
			{
				run_once(20);

			} while (is_stop_ == false);
		});
		thread_.detach();
	}

	void net::stop()
	{
		is_stop_ = true;
	}

	void net::run_once(long timeout_millis)
	{
		zmq_pollitem_t pollitem[2] = {
			{ socket_, 0, ZMQ_POLLIN, 0 },
			{ inproc_, 0, ZMQ_POLLIN, 0 }
		};

		int ret = zmq_poll(&pollitem[0], 2, timeout_millis);
		if (ret == 0)
			return;
		else if (ret < 0)
		{
			std::cout << zmq_strerror(errno) << std::endl;
			return;
		}
		else if (pollitem[0].revents | ZMQ_POLLIN)
		{
			zmq_msg_t msg;
			zmq_msg_init(&msg);
			if (zmq_recvmsg(pollitem[0].socket, &msg, 0) == -1)
			{
				std::cout << zmq_strerror(errno) << std::endl;
			}
			else
			{
				try
				{
					handle_msg(msg);
				}
				catch (const std::exception& e)
				{
					std::cout << e.what() << std::endl;
				}
			}
			zmq_msg_close(&msg);
		}
		else if (pollitem[1].revents | ZMQ_POLLIN)
		{
			zmq_msg_t msg;
			zmq_msg_init(&msg);
			int rc = zmq_recvmsg(pollitem[1].socket, &msg, 0);
			assert(rc != -1);
			assert(*(char*)zmq_msg_data(&msg) == 'K');
			zmq_msg_close(&msg);
			process_msg();
		}
	}

	void net::init()
	{
		zmq_ctx_ = zmq_init(1);
		inproc_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
		if (!zmq_bind(inproc_, "inproc://msg_queue.ipc"))
			throw std::runtime_error(zmq_strerror(errno));
		msg_queue_.init(zmq_ctx_);
	}

	void* net::connect(std::string remote_addr)
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

	void net::do_connect(command &_msg)
	{
		auto engine_id = _msg.net_connect_->engine_id();
		auto remote_addr = _msg.net_connect_->remote_addr();
		auto socket = connect(remote_addr);
		if (socket == nullptr)
		{
			goto connect_failed;
		}

		sys::ping ping;
		ping.set_engine_id(0);

		auto msg = make_message(addr{}, addr{}, ping);
		auto buffer = msg->serialize_as_string();
		int rc = zmq_send(socket, buffer.data(), buffer.size(), 0);
		if (rc == -1)
		{
			goto connect_failed;
		}

		zmq_msg_t zmsg;
		if (!zmq_msg_init(&zmsg))
			goto connect_failed;

		if (zmq_recvmsg(socket, &zmsg, 0) == -1)
			goto connect_failed;

		uint8_t *ptr = (uint8_t*)zmq_msg_data(&zmsg);
		auto type = decode_string(ptr);

	connect_failed:
		sys::net_connect_notify notify;
		notify.set_connected(false);
		*notify.mutable_net_connect() = *_msg.net_connect_;
		send_msg_to_actor_(make_message(addr{}, _msg.net_connect_->from(), notify));
		return;
	}

	void net::do_send(command &_msg)
	{

	}

	void net::do_close(command &_msg)
	{

	}

	void net::handle_msg(zmq_msg_t &_msg)
	{
		assert(handle_msg_);
		handle_msg_(zmq_msg_data(&_msg), zmq_msg_size(&_msg));
	}

	void net::process_msg()
	{
		while (true)
		{
			command _msg;
			if (!msg_queue_.pop_msg(_msg))
				return;
			switch (_msg.type_)
			{
			case command::e_net_connect:
				return do_connect(_msg);
			case command::e_send_msg:
				return do_send(_msg);
			default:
				break;
			}
		}
	}

	

}

