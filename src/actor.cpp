#include "romi.hpp"
namespace romi
{


	actor::actor()
	{
		msg_queue_.check_read();
	}

	actor::~actor() {}

	void actor::init()
	{
		std::cout << "actor init" << std::endl;
	}

	bool actor::dispatch_msg()
	{
		auto item = msg_queue_.read();
		if (!item.first)
			return false;
		dispatch_msg(item.second);
		return msg_queue_.check_read();
	}

	void actor::dispatch_msg(const std::shared_ptr<message_base> &msg)
	{
		auto &func = msg_handles_[msg->type_];
		if (func)
		{
			try
			{
				func(msg);
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
			return;
		}
		else if(msg->get<sys::actor_init>())
		{
			return init();
		}
		std::cout << "Can't find message process handle : " <<
			msg->type_.c_str() << std::endl;
	}

	bool actor::receive_msg(std::shared_ptr<message_base> &&msg)
	{
		std::lock_guard<spinlock> lg(lock_);

		msg_queue_.write(std::move(msg));
		return msg_queue_.flush();
	}

}