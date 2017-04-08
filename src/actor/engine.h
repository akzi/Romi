#pragma once
#include <chrono>
namespace romi
{
	using namespace std::chrono;


	class engine
	{
	public:
		engine();
		~engine();

		template<typename Actor, typename ...Args>
		inline std::enable_if_t<std::is_base_of<actor, Actor>::value, addr> 
			spawn(Args &&...args);

		void add_job(std::function<void()> &&handle);

		void add_job(const std::function<void()> &handle);

		template<typename T>
		std::enable_if_t<std::is_base_of<::google::protobuf::Message, T>::value>
			send(const addr &from, const addr &to, const T &msg);

		void set_config(config cfg);

		void start();

		void stop();


	private:
		using msg_process_handle = actor::msg_process_handle;

		void init_io_engine();

		void init_message_builder();

		void init_message_handle();

		uint64_t gen_actor_id();

		void connect(uint64_t engine_id, const std::string &remote_addr);

		void send(message_base::ptr &&msg);

		void send_to_net(message_base::ptr &msg);

		void add_remote_watcher(addr &from, addr &to);

		void del_remote_watcher(addr &from, addr &to);

		void add_engine_watcher(addr &from, addr &to);

		void del_engine_watcher(addr &from, addr &to);

		void init_actor(actor::ptr &_actor);

		actor::ptr find_actor(addr &_addr);

		void add_actor(actor::ptr &_actor);

		void del_actor(addr &_actor);

		void handle_net_msg(message_base::ptr &msg);

		void resp_pong(const message_base::ptr & msg);

		void handle_pong(const message_base::ptr & msg);

		void check_engine_watcher();

		void ping(uint64_t engine_id);

		void regist_engine();

		void init_nameserver_cluster();

		void init_nameserver_info(addr nameserver_addr);

		void connect_nameserver();

		void find_nameserver_leader();

		msg_process_handle  find_msg_handle(std::string &type);

		void regist_msg_handle(std::string &type, const msg_process_handle &handle);

		void unregist_msg_handle(std::string &type);

		uint64_t gen_req_id();

		config config_;

		struct actors
		{
			~actors()
			{

			}
			std::mutex lock_;
			std::map<addr, actor::ptr, addr_less> actors_;
		} actors_;
		

		std::atomic_bool is_start_{false};
		std::atomic<uint64_t> next_actor_id { 1 };
		
		uint64_t engine_id_ = 0;
		const uint64_t engine_actor_id_ = 0;
		dispatcher_pool dispatcher_pool_;
		timer timer_;
		//watcher
		struct engine_watcher
		{
			struct watcher 
			{
				uint64_t engine_id_ = 0;
				uint64_t timer_id_= 0;
				std::set<addr, addr_less> actors_;
				high_resolution_clock::time_point last_pong_ 
					= high_resolution_clock::now();
			};
			std::mutex locker_;
			std::map<uint64_t, watcher> watchers_;
		} engine_watcher_;


		std::atomic_uint64_t req_id_{ 0 };
		uint64_t timer_id_ = 0;
		net::io_engine io_engine_;
		std::map<std::string, msg_process_handle> msg_handles_;
		std::map<addr, nameserver::nameserver_info, addr_less> nameserver_info_;

		addr nameserver_leader_;
		addr default_nameserver_;
		addr engine_addr_;

		std::unique_ptr<threadpool> threadpool_;
	};
}