#pragma once
namespace romi
{
	struct net_msg
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
		net_msg();
		net_msg(net_msg &&other);
		net_msg &operator = (net_msg &&other);
		~net_msg();
	};

	class msg_queue
	{
	public:
		msg_queue();
		void init(void *zmq_ctx_);
		void push_back(net_msg &&_msg);
		bool pop_msg(net_msg &_msg);
	private:
		void notify();
		void *socket_ = nullptr;
		std::mutex socket_mutex_;
		lock_queue<net_msg> msg_queue_;
	};

	class net
	{
	public:
		using socket = void*;
		net();
		void bind(int port);

		void bind_send_msg_to_actor(std::function<void(message_base::ptr&&)> handle);

		void bind_handle_msg(std::function<void(void*, std::size_t)> handle);

		void send_msg(net_msg &&msg_);

		void start();
		
		void stop();

	private:
		void run();

		void init();

		void* connect(std::string remote_addr);

		void do_connect(net_msg &_msg);

		void do_send(net_msg &_msg);

		void do_close(net_msg &_msg);

		void handle_msg(zmq_msg_t &_msg);

		void process_msg();

		std::thread thread_;
		std::atomic_bool is_stop_{ false };
		void *zmq_ctx_ = nullptr;
		void *req_socket_ = nullptr;
		void *socket_ = nullptr;
		void *inproc_ = nullptr;

		std::thread worker_;
		msg_queue msg_queue_;
		std::function<void(message_base::ptr&&)> send_msg_to_actor_;
		std::function<void(void*, std::size_t)> handle_msg_;
	};
}