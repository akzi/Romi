#pragma once
namespace romi
{
	class dispatcher_pool
	{
	public:
		dispatcher_pool();

		~dispatcher_pool();

		void start(int dispatch_count_);

		void stop();

		void dispatch(std::weak_ptr<actor> &&actor_);

		void increase(int count);
	private:
		bool steal_actor(std::weak_ptr<actor> &_actor);

		const int max_dispather_size_{ 1024 };
		std::atomic_int32_t dispather_size_{ 0 };

		std::atomic_bool is_start_{ false };
		std::atomic_uint32_t dispatch_index_{ 0 };

		std::mutex increase_mutex_;
		std::vector<std::shared_ptr<dispatcher>> dispatchers_;
	};
}