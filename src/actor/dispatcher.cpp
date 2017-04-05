#include "romi.hpp"
namespace romi
{
	dispatcher::dispatcher(steal_actor_handle handle)
		:steal_actor_(handle)
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

	void dispatcher::dispatch(std::weak_ptr<actor> &&actor_)
	{
		std::lock_guard<std::mutex> locker(actor_queue_mutex_);
		actor_queue_.emplace(std::move(actor_));
	}


	bool dispatcher::steal_actor(std::weak_ptr<actor> &_actor)
	{
		return get_actor(_actor);
	}


	bool dispatcher::get_actor(std::weak_ptr<actor> &_actor)
	{
		std::lock_guard<std::mutex> locker(actor_queue_mutex_);
		if (actor_queue_.empty())
			return false;
		_actor = std::move(actor_queue_.front());
		actor_queue_.pop();
		return true;
	}

	void dispatcher::run()
	{
		do
		{
			std::weak_ptr<actor> item;
			if (get_actor(item)|| steal_actor_(item))
			{
				if (const auto _actor = item.lock())
				{
					if (_actor->dispatch_msg())
					{
						dispatch(std::move(item));
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