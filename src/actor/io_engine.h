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
	private:
		void reset();
	};

	class cmd_queue
	{
	public:
		cmd_queue();
		~cmd_queue();
		void init(void *zmq_ctx_, const char *addr_);
		void push_back(command &&_msg);
		bool pop(command &_msg);
		void close();
	private:
		void notify();

		void *socket_ = nullptr;
		std::mutex mutex_;
		lock_queue<command> msg_queue_;
	};


	class io_engine
	{
	public:
		using socket = void*;
		
		io_engine();

		~io_engine();

		void bind(const std::string &addr);

		void bind_send_msg(std::function<void(message_base::ptr&&)> handle);

		void bind_handle_net_msg(std::function<void(void*, std::size_t)> handle);

		void send_cmd(command &&msg_);

		void start();

		void stop();

	private:
		void start_receiver(std::function<void()>, const char *addr_);

		void start_monitor(std::function<void()>, const char *addr_);

		void start_sender(std::function<void()>);


		socket connect(std::string remote_addr);

		void do_connect(command &_msg);

		
		void do_send(command &_msg);

		void process_msg();

		std::string bind_addr_;

		std::atomic_bool is_stop_{ false };

		void *zmq_ctx_ = nullptr;

		std::thread sender_;
		std::thread receiver_;
		std::thread monitor_;

		cmd_queue msg_queue_;

		struct socket_info
		{
			struct socket
			{
				socket(void *sock)
				:socket_(sock){}

				void *socket_;
			};
			std::shared_ptr<socket> socket_;
			std::string remote_addr_;
		};

		std::map<uint64_t, socket_info> sockets_;
		std::function<void(message_base::ptr&&)> send_msg_;
		std::function<void(void*, std::size_t)> handle_msg_;

		//
	};
}
}