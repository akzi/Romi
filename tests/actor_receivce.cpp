#include <iostream>
#include "../src/romi.hpp"

std::atomic_uint64_t msgs_ = 0;
uint64_t last_msg_ = 0;

struct user:romi::actor
{
	user(bool val)
	{
		val_ = val;
	}
	void init()
	{
		std::cout << "init" << std::endl;
		receivce([&](const romi::addr &from, const romi::sys::ping &) {
			msgs_++;
		});

		if(val_)
		set_timer(1000, [&] { 
			std::cout << msgs_ - last_msg_<< std::endl; 
			last_msg_ = msgs_;
			return true; 
		});
	}
	
	bool val_;
};
int actor_receivce()
{
	romi::engine engine;

	romi::config config;
	config.dispatcher_pool_size = 0;

	engine.set_config(config);
	engine.start();

	std::vector<romi::addr> addrs;

	addrs.push_back(engine.spawn<user>(true));
	for (size_t i = 0; i < 1; i++)
	{
		addrs.push_back(engine.spawn<user>(false));
	}
	for (size_t i = 0; i < 1000000000; i++)
	{
		for(auto &itr: addrs)
			engine.send(itr, itr, romi::sys::ping{});
	}

	getchar();
	return 0;
}