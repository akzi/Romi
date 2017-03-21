#include "romi.hpp"
namespace romi
{
	dispatcher_pool::dispatcher_pool()
	{

	}

	dispatcher_pool::~dispatcher_pool()
	{
		if (!is_start_)
			return;
		stop();
	}


	void dispatcher_pool::start(int dispatch_count_ )
	{
		is_start_ = true;
		if (!dispatch_count_)
			dispatch_count_ = std::thread::hardware_concurrency();
		dispatchers_.resize(dispatch_count_);
		for (auto &itr : dispatchers_)
		{
			itr = std::make_shared<dispatcher>([this](std::weak_ptr<actor> &_actor) {
				return steal_actor(_actor);
			});
		}

		for (auto &itr: dispatchers_)
			itr->start();
	}


	void dispatcher_pool::stop()
	{
		for (auto &itr: dispatchers_)
			itr->stop();

		is_start_ = false;
	}

	void dispatcher_pool::dispatch(std::weak_ptr<actor> actor_)
	{
		dispatchers_[++dispatch_index_% dispatchers_.size()]->dispatch(actor_);
	}

	bool dispatcher_pool::steal_actor(std::weak_ptr<actor> &_actor)
	{
		for (size_t i = 0; i < dispatchers_.size(); i++)
		{
			auto index = rand() % dispatchers_.size();
			if (dispatchers_[index]->steal_actor(_actor))
				return true;
		}
		return false;
	}

}