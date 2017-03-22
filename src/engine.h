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
		std::enable_if_t<message_traits<T>::value> 
			send(const addr &from, const addr &to, T &&msg);

		void start();

		void stop();
	private:
		actor_id gen_actor_id();

		void send(message_base::ptr &&msg);

		void send_to_net(message_base::ptr &&msg);

		void add_remote_watcher(addr _actor, event::add_actor_watcher watcher);

		void init_actor(actor::ptr &_actor);

		void add_actor(actor::ptr &_actor);

		struct actors
		{
			spinlock lock_;
			std::map<addr, actor::ptr, addr_less> actors_;
		} actors_;

		std::atomic_bool is_start_{false};
		std::atomic<actor_id> next_actor_id { 1 };
		engine_id engine_id_ = 0;
		dispatcher_pool dispatcher_pool_;
		timer timer_;
	};
}