#pragma once
namespace romi
{
	class dispatcher
	{
	public:
		using steal_actor_handle = 
			std::function<bool(std::weak_ptr<actor> &)>;
		
		dispatcher(steal_actor_handle );
		
		~dispatcher();

		void start();

		void stop();

		void dispatch(std::weak_ptr<actor> &&);
		
		bool steal_actor(std::weak_ptr<actor> &_actor);

	private:
		void run();

		void sleep();
		
		steal_actor_handle  steal_actor_;
		std::atomic_bool is_stop_{ false };
		tp::MPMCBoundedQueue<std::weak_ptr<actor>> queue_;
		std::thread thread_;
	};
}