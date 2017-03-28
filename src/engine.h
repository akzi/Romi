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

		void bind(int port);

		void start();

		void stop();
	private:
		void init();

		actor_id gen_actor_id();

		void send(message_base::ptr &&msg);

		void send_to_net(message_base::ptr &&msg);

		void add_remote_watcher(addr from, addr _actor);

		void del_remote_watcher(addr from, addr _actor);

		void add_engine_watcher(addr from, addr _actor);

		void del_engine_watcher(addr from, addr _actor);

		void init_actor(actor::ptr &_actor);

		void add_actor(actor::ptr &_actor);

		void del_actor(addr &_actor);

		void add_socket(engine_id id, net::socket socket)
		{
			std::lock_guard<std::mutex> locker(sockets_.mutex_);
			sockets_.sockets_[id] = socket;
		}
		void rm_socket(engine_id id)
		{
			std::lock_guard<std::mutex> locker(sockets_.mutex_);
			sockets_.sockets_.erase(id);
		}
		net::socket find_socket(engine_id id)
		{
			std::lock_guard<std::mutex> locker(sockets_.mutex_);
			auto itr = sockets_.sockets_.find(id);
			if (itr != sockets_.sockets_.end())
				return itr->second;
			return nullptr;
		}
		struct actors
		{
			std::mutex lock_;
			std::map<addr, actor::ptr, addr_less> actors_;
		} actors_;
		int port_ = net_bind_port;
		std::atomic_bool is_start_{false};
		std::atomic<actor_id> next_actor_id { 1 };
		engine_id engine_id_ = 0;
		dispatcher_pool dispatcher_pool_;
		timer timer_;
		//watcher
		struct engine_watcher
		{
			std::mutex locker_;
			std::map<engine_id, std::set<addr,addr_less>> watchers_;
		} engine_watcher_;

		//
		net net_;
		struct  sockets
		{
			std::map<engine_id, net::socket> sockets_;
			std::mutex mutex_;
		}sockets_;
	};
}