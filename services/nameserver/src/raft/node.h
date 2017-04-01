#pragma once
namespace romi
{
namespace raft
{
	struct node
	{
		addr addr_;
		std::string raft_id_;
		uint64_t match_index_ = 0;
		uint64_t next_index_ = 0;
		uint64_t heatbeat_inteval_ = 3000;
	};
	enum state
	{
		e_follower,
		e_candidate,
		e_leader
	};
	struct info 
	{
		state state_;

		std::string raft_id_;

		std::vector<node> cluster_;
		std::map<uint64_t,raft::vote_request> vote_requests_;
		std::vector<raft::vote_response> vote_responses_;

		uint64_t append_log_timeout_ = 5000;

		uint64_t current_term_ = 0;
		uint64_t committed_index_ = 0;

		uint64_t last_snapshot_index_ = 0;
		uint64_t last_snapshot_term_ = 0;

		uint64_t last_log_index_ = 0;
		uint64_t last_log_term_ = 0;
		uint64_t last_applied_index_ = 0;

		std::string vote_for_;
		std::string leader_id_;
		std::size_t election_timeout_ = 3000;
		uint64_t election_timer_id_ = 0;

	};
	
}
}