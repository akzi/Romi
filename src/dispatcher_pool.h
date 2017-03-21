#pragma once
namespace romi
{
	class dispatcher_pool
	{
	public:
		dispatcher_pool();
		~dispatcher_pool();
		void dispatch(std::weak_ptr<actor> actor_);
	private:
		std::vector<std::shared_ptr<dispatcher>> dispatchers_;
	};
}