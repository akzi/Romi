#pragma once
#include <ypipe.hpp>
#include "function_traits.hpp"
namespace romi
{
	class actor
	{
	public:
		using ptr = std::shared_ptr<actor>;
	protected:
		actor() {}
		~actor() {}
		
		template<typename Handle>
		void receivce(Handle handle)
		{
			receivce_help(to_function(std::forward<Handle>(handle)));
		}

		template<typename T>
		void send(const addr &to, T &&obj)
		{
			send_msg_(make_message(self_, to, std::forward<T>(obj)));
		}
	private:
		friend class engine;
		friend class dispatcher;

		void dispatch_msg()
		{
			std::shared_ptr<message_base> msg;
			if (msg_queue_.read(&msg))
			{

			}
		}

		void dispath_msg(const std::shared_ptr<message_base> &msg)
		{

		}

		bool receive_msg(std::shared_ptr<message_base> &&msg);

		virtual void init()
		{
			std::cout << "actor init" << std::endl;
		}

		template<typename Message>
		void receivce_help(std::function<void(const addr&, const Message &)> handle)
		{
			auto func = [handle](std::shared_ptr<message_base> msg) {
				Message *message = msg->get<Message>();
				assert(message);
				handle(msg->from_, *message);
			};
			msg_handles_[get_message_type<Message>()] = func;
		}


		addr self_;
		spinlock lock_;
		zmq::ypipe_t<std::shared_ptr<message_base>, msg_queue_granularity> msg_queue_;
		std::function<void(message_base::ptr)> send_msg_;
		std::map<std::string, std::function<void(message_base::ptr)>> msg_handles_;
	};
}