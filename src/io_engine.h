#pragma once
namespace romi
{
namespace sys
{
	struct send_msg
	{
		message_base::ptr message_;
	};
}

namespace net
{
	struct command
	{
		command();
		command(const command &other) = delete;
		command(command &&other);
		command &operator= (command &&other);
		command &operator= (const command &other) = delete;
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

	class cmd_queue
	{
	public:
		cmd_queue();
		void init(void *zmq_ctx_);
		void push_back(command &&_msg);
		bool pop(command &_msg);
	private:
		void notify();

		void *socket_ = nullptr;
		std::mutex socket_mutex_;
		lock_queue<command> msg_queue_;
	};


	class io_engine
	{
	public:
		using socket = void*;
		io_engine();

		void bind(const std::string &addr);

		void bind_send_msg(std::function<void(message_base::ptr&&)> handle);

		void bind_handle_net_msg(std::function<void(void*, std::size_t)> handle);

		void send_cmd(command &&msg_);

		void start();

		void stop();

	private:
		void run();

		void init();

		socket connect(std::string remote_addr);

		void do_connect(command &_msg);

		void do_send(command &_msg);

		void process_msg();

		void handle_monitor_event();

		void handle_cmd_queue_event();

		void handle_msg_event();

		std::thread thread_;
		std::atomic_bool is_stop_{ false };

		void *zmq_ctx_ = nullptr;
		void *req_socket_ = nullptr;
		void *socket_ = nullptr;
		void *cmd_queue_inproc_ = nullptr;
		void *monitor_ = nullptr;

		std::thread worker_;
		cmd_queue msg_queue_;

		std::map<uint64_t, void *> sockets_;
		std::function<void(message_base::ptr&&)> send_msg_;
		std::function<void(void*, std::size_t)> handle_msg_;
	};
}
}