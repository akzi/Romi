#pragma once
namespace romi
{
	class spinlock
	{
	public:
		spinlock();
		~spinlock();
		void lock();

		bool try_lock();

		void unlock();
	private:
		std::atomic_flag lock_;
	};
}