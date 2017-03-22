#include "romi.hpp"

namespace romi
{
	spinlock::spinlock()
	{
		lock_.clear(std::memory_order_release);
	}

	
	bool spinlock::try_lock()
	{
		return !lock_.test_and_set(std::memory_order_acquire);
	}

	void spinlock::lock()
	{
		while (lock_.test_and_set(std::memory_order_acquire));
	}

	void spinlock::unlock()
	{
		lock_.clear(std::memory_order_release);
	}

}