#include "romi.hpp"

namespace romi
{
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

	void msg_queue::push_back(net_msg &&_msg)
	{
		if (msg_queue_.push(std::forward<net_msg>(_msg)) == 1)
		{
			notify();
		}
	}

	bool msg_queue::pop_msg(net_msg &_msg)
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

	void net::send_msg(net_msg &&msg_)
	{
		msg_queue_.push_back(std::move(msg_));
	}

	void net::start()
	{
		thread_ = std::thread([this] {
			run();
		});
		thread_.detach();
	}

	void net::stop()
	{
		is_stop_ = true;
	}

	void net::run()
	{
		zmq_pollitem_t pollitem[2] = {
			{ socket_, 0, ZMQ_POLLIN, 0 },
			{ inproc_, 0, ZMQ_POLLIN, 0 }
		};

		do
		{
			bool retired = false;
			int ret = zmq_poll(&pollitem[0], 2, 10);
			if (ret == 0)
				continue;
			else if (ret < 0)
			{
				std::cout << zmq_strerror(errno) << std::endl;
			}
			else
				if (pollitem[0].revents | ZMQ_POLLIN)
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

		} while (is_stop_ == false);
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

	void net::do_connect(net_msg &_msg)
	{

	}

	void net::do_send(net_msg &_msg)
	{

	}

	void net::do_close(net_msg &_msg)
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
			net_msg _msg;
			if (!msg_queue_.pop_msg(_msg))
				return;
		}
	}
}

