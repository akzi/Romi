#include "romi.hpp"
namespace romi
{
	bool actor::receive_msg(std::shared_ptr<message_base> &&msg)
	{
		std::lock_guard<spinlock> lg(lock_);

		msg_queue_.write(std::move(msg), false);
		return msg_queue_.flush();
	}
}