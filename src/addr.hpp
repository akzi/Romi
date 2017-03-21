#pragma once
namespace romi
{
	using engine_id = uint64_t;
	struct addr
	{
		struct less
		{
			bool operator()(const addr &left, const addr &right)
			{
				return left.engine_id_ < right.engine_id_ ||
					left.actor_id_ < right.actor_id_;
			}
		};
		engine_id  engine_id_;
		uint64_t actor_id_;
		std::weak_ptr<class actor> actor_;
	};
}