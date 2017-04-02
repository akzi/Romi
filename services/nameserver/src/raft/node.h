#pragma once
namespace romi
{
namespace raft
{
	
	struct peer
	{
		std::string net_addr_;
		addr addr_;
		std::string raft_id_;
		uint64_t match_index_ = 0;
		uint64_t next_index_ = 0;
		uint64_t heatbeat_inteval_ = 3000;
	};
	
	class node : public actor
	{
	public:
		enum state
		{
			e_follower,
			e_candidate,
			e_leader
		};

		node();
	protected:
		virtual void init_node();
	private:
		void receive(const addr &from, const romi::raft::vote_request &message);

		void receive(const addr &from, const raft::install_snapshot_response &resp);

		void receive(const addr &from, const raft::install_snapshot_request &resp);

		void receive(const addr &from, const raft::vote_response& resp);

		void receive(const addr &from, const raft::replicate_log_entries_request &resp);

		void receive(const addr &from, const raft::replicate_log_entries_response &req);

		//
		void connect_node();

		void set_election_timer();

		void do_election();

		void cancel_election_timer();

		void set_down(uint64_t term);

		void become_leader();

		void replicate_log_entry();

		void add_log_entries(uint64_t next_index, uint32_t max_bytes, 
			raft::replicate_log_entries_request &req);

		uint64_t gen_req_id();

		int majority();

		//
		uint64_t req_id_ = 0;

		state state_;

		std::string raft_id_;

		std::vector<peer> peers_;
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

		
		raft_log log_;
	};
	
}
}