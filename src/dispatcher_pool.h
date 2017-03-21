#pragma once
namespace romi
{
	class dispatcher_pool
	{
	public:
		dispatcher_pool();

		~dispatcher_pool();

		void start(int dispatch_count_ = dispatcher_pool_size);

		void stop();

		void dispatch(std::weak_ptr<actor> actor_);

	private:
		bool steal_actor(std::weak_ptr<actor> &_actor);

		std::atomic_bool is_start_{ false };
		std::atomic_uint32_t dispatch_index_{ 0 };
		std::vector<std::shared_ptr<dispatcher>> dispatchers_;
	};
}