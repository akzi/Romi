#pragma once
namespace romi
{
	struct config
	{
		std::string engine_name_;
		std::string nameserver_addr_;
		int zeromq_bind_port_ = 10927;
		int net_heartbeart_interval = 1000;
		int dispatcher_pool_size = 0;// CPU core count
	};
}