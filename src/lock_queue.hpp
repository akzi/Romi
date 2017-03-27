#pragma once
#pragma once
#include <queue>
#include <mutex>
namespace romi
{
	template<typename T>
	class lock_queue
	{
	public:
		lock_queue()
		{
		}

		std::size_t push(T &&item)
		{
			std::lock_guard<std::mutex> locker(mtex_);
			queue_.push(std::forward<T>(item));
			return queue_.size();
		}

		bool pop(T &job, int timeout_mills = 0)
		{
			std::unique_lock<std::mutex> locker(mtex_);
			if (queue_.empty())
				return false;
			job = std::move(queue_.front());
			queue_.pop();
			return true;
		}
		std::size_t jobs()
		{
			std::unique_lock<std::mutex> locker(mtex_);
			return queue_.size();
		}
		bool emtry()
		{
			std::unique_lock<std::mutex> locker(mtex_);
			return queue_.empty();
		}
	private:
		std::mutex mtex_;
		std::queue<T> queue_;
	};
}