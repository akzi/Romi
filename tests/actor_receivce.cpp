#include <iostream>
#include "../src/romi.hpp"


struct user:romi::actor
{
	user()
	{
		receivce([](const romi::addr &from, const std::string &msg) {

		});

		send(romi::addr(), std::string());
		send(romi::addr(), 0l);
	}
};
int main()
{
	std::cout << sizeof(std::shared_ptr<romi::actor>) << std::endl;
	user u;
	return 0;
}