#include "romi.hpp"

namespace romi
{



	template<typename T, typename D>
	struct duration_caster
	{
		T operator()(D d)
		{
			return std::chrono::duration_cast<T>(d);
		}
	};

	template<typename CLOCK>
	struct time_pointer
	{
		typename CLOCK::time_point operator()()
		{
			return CLOCK::now();
		}
	};

	int64_t timer_manager::do_timer()
	{
		if (empty())
			return 0;
		auto itr = begin();
		duration_caster<milliseconds,
			high_resolution_clock::duration> caster;

		time_pointer<high_resolution_clock> time_point;
		auto now = time_point();
		while (size() &&
			(caster(itr->first.time_since_epoch()) <=
				caster(now.time_since_epoch())))
		{
			timer_callback timer = itr->second;
			erase(itr);
			bool repeat = timer.handle_();
			now = time_point();
			if (repeat)
			{
				auto point = now + high_resolution_clock::
					duration(milliseconds(timer.timeout_));
				insert(std::make_pair(point, timer));
			}
			if (size())
				itr = begin();
			else
				return 0;
		}
		return (caster(itr->first.time_since_epoch()) -
			caster(now.time_since_epoch())).count();
	}

	romi::timer_id timer_manager::set_timer(std::size_t timeout, timer_handle &&handle)
	{
		auto timer_point = std::chrono::high_resolution_clock::now()
			+ std::chrono::high_resolution_clock::
			duration(std::chrono::milliseconds(timeout));
		timer_callback callback;
		callback.handle_ = std::move(handle);
		callback.timeout_ = timeout;
		callback.timer_id_ = ++next_id_;
		insert(std::make_pair(timer_point, callback));
		return next_id_;
	}

	void timer_manager::cancel_timer(std::size_t id)
	{
		for (auto itr = begin(); itr != end(); ++itr)
		{
			if (id == itr->second.timer_id_)
			{
				erase(itr);
				return;
			}
		}
	}

	timer::timer() {}

	timer::~timer() {}

	romi::timer_id timer::set_timer(std::size_t timeout, timer_handle &&timer_callback)
	{
		std::lock_guard<std::mutex> lg(mutex_);
		auto id = timer_manager_.set_timer(timeout, std::move(timer_callback));
		cv_.notify_one();
		return id;
	}

	void timer::cancel_timer(std::size_t id)
	{
		return timer_manager_.cancel_timer(id);
	}

	void timer::start()
	{
		worker_ = std::thread([this] {
			do
			{
				std::unique_lock<std::mutex> lg(mutex_);
				auto delay = timer_manager_.do_timer();
				if (!delay)
					delay = 1000;
				cv_.wait_for(lg, std::chrono::milliseconds(delay));

			} while (!is_stop);
		});
	}

	void timer::stop()
	{
		is_stop = true;
		cv_.notify_one();
		worker_.join();
	}
}



