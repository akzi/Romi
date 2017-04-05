#pragma once
namespace romi
{
	struct addr_less
	{
		bool operator()(const addr &left, const addr &right)const
		{
			return left.engine_id() < right.engine_id()
				|| left.actor_id() < right.actor_id();
		}
	};
}