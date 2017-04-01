#pragma once
struct nameserver_config
{
	struct node
	{
		//format eg: tpc://127.0.0.1:9001
		std::string addr_;
		uint64_t engine_id_;
		uint64_t actor_id_;
	};
	std::list<node> nameserver_cluster_;
};