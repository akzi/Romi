#pragma once
namespace romi
{
	class engine
	{
	public:
		engine();
		~engine();

		template<typename Actor, typename ...Args>
		inline std::enable_if_t<std::is_base_of<actor, Actor>::value, addr> 
			spawn(Args &&...args);

		template<typename T>
		std::enable_if_t<std::is_base_of<::google::protobuf::Message, T>::value>
			send(const addr &from, const addr &to, const T &msg);

		void set_config(config cfg);

		void start();

		void stop();
	private:

		void ping(uint64_t engine_id)
		{
			sys::ping ping;
			ping.set_engine_id(engine_id);
			addr from;
			addr to;
			from.set_engine_id(engine_id_);
			from.set_actor_id(0);
			to.set_engine_id(engine_id);
			to.set_actor_id(0);
			auto msg = make_message(from,to, ping);
			send_to_net(msg);
		}
		void init();

		uint64_t gen_actor_id();

		void send(message_base::ptr &&msg);

		void send_to_net(message_base::ptr &msg);

		void add_remote_watcher(addr from, addr _actor);

		void del_remote_watcher(addr from, addr _actor);

		void add_engine_watcher(addr from, addr _actor);

		void del_engine_watcher(addr from, addr _actor);

		void init_actor(actor::ptr &_actor);

		actor::ptr find_actor(addr &_addr);

		void add_actor(actor::ptr &_actor);

		void del_actor(addr &_actor);

		void handle_msg(message_base::ptr &msg);

		struct actors
		{
			std::mutex lock_;
			std::map<addr, actor::ptr, addr_less> actors_;
		} actors_;
		
		config config_;

		std::atomic_bool is_start_{false};
		std::atomic<uint64_t> next_actor_id { 1 };
		
		uint64_t engine_id_ = 0;

		dispatcher_pool dispatcher_pool_;
		timer timer_;
		//watcher
		struct engine_watcher
		{
			std::mutex locker_;
			std::map<uint64_t, std::set<addr,addr_less>> watchers_;
		} engine_watcher_;

		io_engine io_engine_;
	};
}