#include "romi.hpp"
namespace romi
{
	dispatcher::dispatcher(steal_actor_handle handle)
		:steal_actor_(handle),
		queue_(dispatcher_queue_size)
	{
		
	}

	dispatcher::~dispatcher()
	{
		if (is_stop_)
			return;
		stop();
	}


	void dispatcher::start()
	{
		thread_ = std::thread([this] {
			run();
		});
	}

	void dispatcher::stop()
	{
		is_stop_ = true;
		thread_.join();
	}

	void dispatcher::dispatch(std::weak_ptr<actor> actor_)
	{
		if (!queue_.push(std::move(actor_)))
			throw std::runtime_error("queue full");
	}


	bool dispatcher::steal_actor(std::weak_ptr<actor> &_actor)
	{
		return queue_.pop(_actor);
	}

	void dispatcher::run()
	{
		do
		{
			std::weak_ptr<actor> item;
			if (queue_.pop(item) || steal_actor_(item))
			{
				if (const auto _actor = item.lock())
				{
					if (_actor->dispatch_msg())
					{
						dispatch(item);
					}
				}
			}
			else
			{
				sleep();
			}

		} while (!is_stop_);
	}

	void dispatcher::sleep()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

}