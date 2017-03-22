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
		receivce([&](const romi::addr &from, const std::string &) {
			msgs_++;
		});

		if(val_)
		set_timer(1000, [&] { 
			std::cout << msgs_ - last_msg_<< std::endl; 
			last_msg_ = msgs_;
			return true; 
		});

		using romi::sys::add_watcher;
		using romi::sys::del_watcher;

		add_watcher(add_watcher{romi::addr()});
		del_watcher(del_watcher{ romi::addr() });
		
	}
	
	bool val_;
};
void ypipe_test();
int main()
{
	ypipe_test();

	romi::engine engine;
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
		engine.send(itr, itr, std::string("msg"));
	}

	getchar();
	return 0;
}