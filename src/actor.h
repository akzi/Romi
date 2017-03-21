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

	private:
		virtual void init();

		bool dispatch_msg();

		void dispatch_msg(const std::shared_ptr<message_base> &msg);

		bool receive_msg(std::shared_ptr<message_base> &&msg);

		template<typename Message>
		inline void receivce_help(std::function<void(const addr&, const Message &)>);

		friend class engine;
		friend class dispatcher;

		addr addr_;
		spinlock lock_;
		ypipe<std::shared_ptr<message_base>> msg_queue_;
		std::function<void(message_base::ptr)> send_msg_;
		std::map<std::string, std::function<void(message_base::ptr)>> msg_handles_;
	};
}