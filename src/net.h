#pragma once
namespace romi
{
	namespace sys
	{
		struct send_msg
		{
			std::string buffer_;
			uint64_t engine_id_;
		};
	}

	struct command
	{
		command();
		command(command &&other);
		command &operator= (command &&other);
		~command();
		enum type
		{
			e_null = 0,
			e_net_connect,
			e_send_msg,
		} type_;
		union 
		{
			sys::net_connect *net_connect_;
			sys::send_msg *send_msg_;
		};
	};

	class msg_queue
	{
	public:
		msg_queue();
		void init(void *zmq_ctx_);
		void push_back(command &&_msg);
		bool pop_msg(command &_msg);
	private:
		void notify();

		void *socket_ = nullptr;
		std::mutex socket_mutex_;
		lock_queue<command> msg_queue_;
	};

	class net
	{
	public:
		using socket = void*;
		net();
		void bind(int port);

		void bind_send_msg_to_actor(std::function<void(message_base::ptr&&)> handle);

		void bind_handle_msg(std::function<void(void*, std::size_t)> handle);

		void send_msg(command &&msg_);

		void start();
		
		void stop();

		void run_once(long timeout_millis);

	private:

		void init();

		void* connect(std::string remote_addr);

		void do_connect(command &_msg);

		void do_send(command &_msg);

		void do_close(command &_msg);

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

		std::map<uint64_t, void *> sockets_;
		std::function<void(message_base::ptr&&)> send_msg_to_actor_;
		std::function<void(void*, std::size_t)> handle_msg_;
	};
}