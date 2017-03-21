#include <iostream>
#include "../src/romi.hpp"


struct user:romi::actor
{
	void init()
	{
		receivce([](const romi::addr &from, const std::string &msg) {
			std::cout << msg.c_str() << std::endl;
		});
		std::cout << "user init" << std::endl;
	}
};
int main()
{
	romi::engine engine;
	engine.start();

	auto addr = engine.spawn<user>();
	engine.send(addr, addr, std::string("hello world"));

	getchar();
	return 0;
}