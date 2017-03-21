#pragma once
namespace romi
{
	class spinlock
	{
	public:
		spinlock()
		{
		}
		void lock()
		{
			while (std::atomic_flag_test_and_set_explicit(&lock_,
				std::memory_order_acquire));
		}
		bool try_lock()
		{
			return !std::atomic_flag_test_and_set_explicit(&lock_, 
				std::memory_order_acquire);
		}
		void unlock()
		{
			std::atomic_flag_clear_explicit(&lock_, 
				std::memory_order_release);
		}
	private:
		std::atomic_flag lock_;
	};
}