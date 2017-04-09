#pragma once
namespace romi
{
	struct nameserver_config
	{

		uint64_t engine_id_;
		uint64_t actor_id_;
		std::string store_path_;
		std::string snapshot_dir_;
		std::string wal_path_;
		raft::node::config raft_node_cfg_;
	};
}
