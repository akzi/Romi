#pragma once
namespace romi
{
	class spinlock
	{
	public:
		void lock();
		bool try_lock();
		void unlock();
	private:
		std::atomic_flag lock_;
	};
}