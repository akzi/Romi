#pragma once
namespace romi
{
	using engine_id = uint64_t;
	using actor_id = uint64_t;

	struct addr
	{
		engine_id  engine_id_ = 0;
		actor_id actor_id_ = 0;
		std::weak_ptr<class actor> actor_;
	};

	struct addr_less
	{
		bool operator()(const addr &left, const addr &right)const
		{
			return left.engine_id_ < right.engine_id_ ||
				left.actor_id_ < right.actor_id_;
		}
	};
}