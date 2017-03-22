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
		void receivce(Handle handle);

		template<typename T>
		void send(const addr &to, T &&obj);

		timer_id set_timer(std::size_t mills, timer_handle &&);

		void cancel_timer(timer_id id);

		void add_watcher(const event::add_actor_watcher& );
		void del_watcher(const event::del_actor_watcher& );

		void add_watcher(const event::add_engine_watcher& );
		void del_watcher(const event::del_engine_watcher& );


	private:
		virtual void init();

		bool dispatch_msg();

		void dispatch_msg(const std::shared_ptr<message_base> &msg);

		void timer_expire(timer_id id);

		bool receive_msg(std::shared_ptr<message_base> &&msg);

		template<typename Message>
		inline void receivce_help(std::function<void(const addr&, const Message &)>);

		friend class engine;
		friend class dispatcher;

		addr addr_;
		//msg
		spinlock lock_;
		ypipe<std::shared_ptr<message_base>> msg_queue_;		
		std::map<std::string, std::function<void(message_base::ptr)>> msg_handles_;
		std::function<void(message_base::ptr)> send_msg_;
		//timer
		std::function<timer_id(addr, std::size_t, timer_id)> set_timer_;
		std::function<void(timer_id)> cancel_timer_;
		timer_id timer_id_ = 0;
		std::map<timer_id, std::pair<timer_id, timer_handle>> timer_handles_;


	};
}