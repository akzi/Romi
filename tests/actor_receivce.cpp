#include <iostream>
#include "../src/romi.hpp"


struct user:romi::actor
{
	void init()
	{
		std::cout << "init" << std::endl;
		receivce([](const romi::addr &from, const std::string &msg) {
			std::cout << msg.c_str() << std::endl;
		});
		auto id = set_timer(100, [] { std::cout << "." << std::endl; return true; });
		set_timer(1000, [=] { cancel_timer(id); return false; });

		using romi::event::add_actor_watcher;
		using romi::event::del_actor_watcher;

		add_watcher(add_actor_watcher{romi::addr()});
		del_watcher(del_actor_watcher{ romi::addr() });
		
	}
};
int main()
{
	romi::engine engine;
	engine.start();

	auto addr = engine.spawn<user>();
	engine.send(addr, addr, std::string("msg"));

	getchar();
	return 0;
}