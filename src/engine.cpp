#include "romi.hpp"
namespace romi
{
	using lock_guad = std::lock_guard<std::mutex>;
	engine::engine()
	{

	}
	engine::~engine()
	{

	}
	void engine::send(std::shared_ptr<message_base> &&msg)
	{
		if (const auto _actor = msg->to_.actor_.lock())
		{
			if (!_actor->receive_msg(std::move(msg)))
			{

			}
		}
	}
}