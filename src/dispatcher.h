#pragma once
namespace romi
{
	class dispatcher
	{
	public:
		dispatcher();
		~dispatcher();
		void dispatch(std::weak_ptr<actor> actor_);
	private:
		std::thread worker_;
	};
}