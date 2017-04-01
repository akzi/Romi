#include "romi.hpp"
#include "nameserver.h"


int main()
{
	romi::engine engine;
	romi::config config;
	config.engine_id_ = 1;
	config.engine_name_ = "nameserver";
	config.net_bind_addr_ = config.nameserver_addr_;
	config.is_nameserver_ = true;

	engine.set_config(config);
	engine.start();

	engine.spawn<romi::nameserver>();

	getchar();
	return 0;
}