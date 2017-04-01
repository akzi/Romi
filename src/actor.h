#pragma once
namespace romi
{
	class actor
	{
	public:
		using ptr = std::shared_ptr<actor>;
	protected:
		actor();

		virtual ~actor();

		template<typename Handle>
		void receive(Handle handle);

		template<typename T>
		void send(const addr &to, T &&obj);

		void send(message_base::ptr &&msg);

		timer_id set_timer(std::size_t mills, timer_handle &&);

		void cancel_timer(timer_id id);

		void watch(addr actor_);

		void cancel_watch(addr actor_);
		
		void close();

		addr get_addr();

		addr get_engine_addr();

		addr get_nameserver_addr();

		void connect(sys::net_connect &msg);

		std::size_t get_actor_size();

		uint32_t get_dispatcher_size();

		void increase_dispather(int count_);
	private:
		virtual void init();

		void init_msg_process_handle();

		bool dispatch_msg();

		void apply_msg(const message_base::ptr &msg);

		void timer_expire(timer_id id);

		std::size_t receive_msg(message_base::ptr &&msg);

		template<typename Message>
		inline void receive_help(std::function<void(const addr&, const Message &)>);

		friend class engine;
		friend class dispatcher;

		using msg_process_handle = std::function<void(const message_base::ptr& )>;

		addr addr_;

		addr nameserver_addr_;

		lock_queue<message_base::ptr> msg_queue_;

		std::map<std::string, msg_process_handle> msg_handles_;

		std::function<void(message_base::ptr)> send_msg_;
		//timer
		timer_id timer_id_ = 0;
		std::function<void(timer_id)> cancel_timer_;
		std::function<timer_id(addr, std::size_t, timer_id)> set_timer_;
		std::map<timer_id, std::pair<timer_id, timer_handle>> timer_handles_;

		//close
		bool is_close_ = false;
		std::function<void(addr&)> close_callback_;

		//Observer 
		std::set<addr, addr_less> observers_;

		//watcher
		std::set<addr, addr_less> watchers_;

		std::function<void(addr&, addr&)> add_watcher_;
		std::function<void(addr&, addr&)> cancel_watch_;

		//config functions
		std::function<void(int)> increase_dispather_;
		std::function<std::size_t()> get_actor_size_;
		std::function<uint32_t()> get_dispatcher_size_;
	};
}