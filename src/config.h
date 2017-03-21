#pragma once
namespace romi
{
	enum
	{
		dispatcher_queue_granularity = 128,
		
		msg_queue_granularity = 128,

		//max actor size 
		dispatcher_queue_size = 1024,

		//0 for  hardware_concurrency
		dispatcher_pool_size = 1
	};
}