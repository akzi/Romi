#include "romi.hpp"
namespace romi
{
	dispatcher_pool::dispatcher_pool()
	{
		dispatchers_.resize(max_dispather_size_);
	}

	dispatcher_pool::~dispatcher_pool()
	{
		if (!is_start_)
			return;
		stop();
	}

	void dispatcher_pool::start(int size )
	{
		is_start_ = true;
		if (!size)
			size = std::thread::hardware_concurrency();
		
		for (int i = 0; i < size; i++)
		{
			 dispatchers_[i] = std::make_shared<dispatcher>(
				 [this](std::weak_ptr<actor> &_actor) {
				return steal_actor(_actor);
			 });
			 dispatchers_[i]->start();
			 ++dispather_size_;
		}
	}


	void dispatcher_pool::stop()
	{
		for (auto &itr : dispatchers_)
		{
			if(itr)
				itr->stop();
		}
			

		is_start_ = false;
	}

	void dispatcher_pool::dispatch(std::weak_ptr<actor> &&actor_)
	{
		auto dispatcher = dispatchers_[++dispatch_index_% dispather_size_];
		dispatcher->dispatch(std::move(actor_));
	}


	void dispatcher_pool::increase(int count)
	{
		std::lock_guard<std::mutex> locker(increase_mutex_);
		for (int i = 0; i < max_dispather_size_ && 0 < count; i++)
		{
			if (dispatchers_[i] == nullptr)
			{
				dispatchers_[i] = std::make_shared<dispatcher>(
					[this](std::weak_ptr<actor> &_actor) {
					return steal_actor(_actor);
				});
				dispatchers_[i]->start();
				count--;
				dispather_size_++;
			}
		}
	}


	uint32_t dispatcher_pool::size()
	{
		return dispather_size_;
	}

	bool dispatcher_pool::steal_actor(std::weak_ptr<actor> &_actor)
	{
		for (size_t i = 0; i < dispather_size_; i++)
		{
			auto index = rand() % dispather_size_;
			if (dispatchers_[index]->steal_actor(_actor))
				return true;
		}
		return false;
	}

}