#pragma once
#include <chrono>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace romi
{
	using namespace std::chrono;
	using timer_id = std::size_t;

	using timer_handle = std::function<bool()>;
	struct timer_callback
	{
		std::size_t timer_id_;
		std::size_t timeout_;
		timer_handle handle_;
	};

	class timer_manager :
		public std::multimap<
		high_resolution_clock::time_point, 
		timer_callback>
	{
	public:
		int64_t do_timer();
		timer_id set_timer(std::size_t timeout, timer_handle &&);
		void cancel_timer(std::size_t id);
	private:
		std::size_t next_id_ = 0;
	};

	class timer
	{
	public:
		timer();
		~timer();
		timer_id set_timer(std::size_t timeout, timer_handle &&);
		void cancel_timer(std::size_t id);
		void start();
		void stop();
	private:
		timer_manager timer_manager_;
		std::mutex mutex_;
		std::condition_variable cv_;
		std::thread worker_;
		bool is_stop = false;
	};
}
