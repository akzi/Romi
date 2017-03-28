#pragma once
namespace romi
{
	struct msg
	{
		enum type
		{
			e_connect = 1,
			e_send,
			e_close
		}type_;

		union 
		{
			sys::net_connect *connect_;
			sys::net_send *send_;
			sys::net_close *close_;
		};
		msg()
		{
			memset(this, 0, sizeof(msg));
		}
		msg(msg &&other)
		{
			memcpy(this, &other, sizeof(msg));
			memset(&other, 0, sizeof(msg));
		}
		msg &operator = (msg &&other)
		{
			memcpy(this, &other, sizeof(msg));
			memset(&other, 0, sizeof(msg));
			return *this;
		}
		~msg()
		{
			if (type_ == e_connect)
				delete connect_;
			else if (type_ == e_close)
				delete close_;
			else if (type_ == e_send)
				delete send_;
		}
	};

	class msg_queue
	{
	public:
		msg_queue()
		{
		}
		void init(void *zmq_ctx_)
		{
			socket_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
			if (!zmq_connect(socket_, "inproc://msg_queue.ipc"))
				throw std::runtime_error(zmq_strerror(errno));
			zmq_msg_t msg;
			zmq_msg_init_size(&msg, 1);
			*reinterpret_cast<char*>(zmq_msg_data(&msg)) = 'K';
		}
		void push_back(msg &&_msg)
		{
			if (msg_queue_.push(std::forward<msg>(_msg)) == 1)
			{
				notify();
			}
		}
		bool pop_msg(msg &_msg)
		{
			return msg_queue_.pop(_msg);
		}
	private:
		void notify()
		{
			std::lock_guard<std::mutex> locker(socket_mutex_);
			if (!zmq_send(socket_, "K", 1, 0))
			{
				throw std::runtime_error(zmq_strerror(errno));
			}
		}
		void *socket_ = nullptr;
		std::mutex socket_mutex_;
		lock_queue<msg> msg_queue_;
	};

	class net
	{
	public:
		using socket = void*;
		net()
		{

		}
		void bind(int port)
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
		void send_msg(msg &&msg_)
		{
			msg_queue_.push_back(std::move(msg_));
		}
		void start()
		{
			thread_ = std::thread([this]{
				run();
			});
			thread_.detach();
		}
		
		void stop()
		{
			is_stop_ = true;
		}

	private:
		void run()
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

		void init()
		{
			zmq_ctx_ = zmq_init(1);
			inproc_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
			if (!zmq_bind(inproc_, "inproc://msg_queue.ipc"))
				throw std::runtime_error(zmq_strerror(errno));
			msg_queue_.init(zmq_ctx_);
		}

		void* connect(std::string remote_addr)
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
		void do_connect(msg &_msg)
		{
			assert(actor_msg_callback_);
			auto s = connect(_msg.connect_->remote_addr_);
			auto addr = _msg.connect_->actor_;
			actor_msg_callback_(make_message(addr, addr, 
				sys::net_connect_result{*_msg.connect_, s}));
		}

		void do_send(msg &_msg)
		{
			sys::net_send &send_= *_msg.send_;
			int rc = zmq_send(send_.socket_, send_.buffer_.data(), 
				send_.buffer_.size(), 0);
			if (rc == -1)
			{
				assert(actor_msg_callback_);
				actor_msg_callback_(make_message(addr{}, send_.from_actor_, 
					sys::net_send_failed{send_, zmq_strerror(errno)}));
			}
		}

		void do_close(msg &_msg)
		{
			int rc = zmq_close(_msg.close_->socket_);
			if (rc != 0)
				std::cout << zmq_strerror(errno) << std::endl;
		}

		void handle_msg(zmq_msg_t &_msg)
		{
			assert(handle_msg_);
			handle_msg_(zmq_msg_data(&_msg), zmq_msg_size(&_msg));
		}

		void process_msg()
		{
			while(true)
			{
				msg _msg;
				if (!msg_queue_.pop_msg(_msg))
					return;
				switch (_msg.type_)
				{
				case msg::e_connect:
					do_connect(_msg);
					break;
				case msg::e_send:
					do_send(_msg);
					break;
				case msg::e_close:
					do_close(_msg);
					break;
				}
			}
		}

		std::thread thread_;
		std::atomic_bool is_stop_{ false };
		void *zmq_ctx_ = nullptr;
		void *req_socket_ = nullptr;
		void *socket_ = nullptr;
		void *inproc_ = nullptr;

		std::thread worker_;
		msg_queue msg_queue_;
		std::function<void(message_base::ptr)> actor_msg_callback_;
		std::function<void(void*, std::size_t)> handle_msg_;
	};
}