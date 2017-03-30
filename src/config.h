#pragma once
namespace romi
{
	struct config
	{
		bool is_nameserver_ = false;
		//0 for get engine from nameserver
		uint64_t engine_id_ = 0;
		std::string engine_name_;
		std::string nameserver_addr_ = "tcp://127.0.0.1:10095";
		//nameserver is first engine in the cluster;
		uint64_t nameserver_engine_id_ = 0;
		std::string net_bind_addr_ = "tcp://127.0.0.1:10096";
		int net_heartbeart_interval = 1000;
		//milliseconds
		int net_heartbeart_timeout = 5000;
		//0 for CPU core count
		int dispatcher_pool_size = 0;
	};
}