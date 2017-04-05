#pragma once
#include <fstream>
#include "romi.hpp"
#include "romi.raft.pb.h"
#include "raft_log.h"
#include "raft_log.h"


namespace romi
{
namespace raft
{
	class node : public actor
	{
	public:
		struct config 
		{
			struct node_info 
			{
				std::string net_addr_;
				addr addr_;
				std::string raft_id_;
			};
			std::list<node_info> others_;
			std::string raft_id_;
			std::string store_path_;
		};
		enum state
		{
			e_follower,
			e_candidate,
			e_leader
		};
		node();
	protected:
		void init_node(config cfg_);

		bool is_leader();

		uint64_t replicate(const std::string &msg);

	protected:
		virtual void repicate_callback(const std::string & data, uint64_t index);

		virtual void commit_callback(uint64_t index);

		virtual void no_leader_callback();

		virtual std::string get_snapshot_file(uint64_t index);

		virtual void make_snapshot_callback(uint64_t last_include_term, uint64_t last_include_index);

		virtual void new_snapshot_callback(raft::snapshot_info info, std::string &filepath);

		virtual void receive_snashot_file_failed(std::string &filepath);

		virtual void receive_snashot_file_success(std::string &filepath);

		virtual bool support_snapshot();
	private:
		struct peer
		{
			std::string net_addr_;
			addr addr_;
			std::string raft_id_;
			uint64_t match_index_ = 0;
			uint64_t next_index_ = 0;
			int64_t heartbeat_inteval_ = 3000;
			high_resolution_clock::time_point last_heartbeat_time_;
			uint64_t heartbeat_timer_id_ = 0;
			std::set<uint64_t> req_ids_;
			std::ifstream snapshot_;
			raft::snapshot_info snapshot_info_;
		};

		void receive(const addr &from, const romi::raft::vote_request &message);

		void receive(const addr &from, const raft::vote_response& resp);

		void receive(const addr &from, const raft::replicate_log_entries_request &resp);

		void receive(const addr &from, const raft::replicate_log_entries_response &req);

		void receive(const addr &from, const raft::install_snapshot_response &resp);

		void receive(const addr &from, const raft::install_snapshot_request &req);

		//
		void connect_node();

		void set_election_timer();

		void reset_election_timer();

		void do_election();

		void cancel_election_timer();

		void set_down(uint64_t term);

		void become_leader();

		void replicate_log_entry();

		void replicate_log_entry(peer &_peer);

		void add_log_entries(uint64_t next_index, uint32_t max_bytes, 
			raft::replicate_log_entries_request &req);

		log_entry get_log_entry(uint64_t index);

		uint64_t gen_req_id();

		int majority();

		void log_truncate_suffix(uint64_t index);

		void response(const addr &from, const raft::replicate_log_entries_response &resp);

		void write_raft_log(const std::list<std::pair<uint64_t, std::string>> &);

		void write_raft_log(const raft::log_entry &entry);

		std::list<raft::log_entry> get_log_entries(uint64_t index, uint64_t count);

		void check_commit_log_entries(const std::string &raft_id, uint64_t match_index);
		
		bool try_install_snapshot(peer & _peer);

		void do_install_snapshot(peer &_peer ,const std::string &snashot);
	
		void send_install_snapshot(peer &_peer);

		void set_heartbeat_timer(peer &_peer);

		uint64_t req_id_ = 0;

		state state_;

		std::string raft_id_;

		std::map<addr, peer, addr_less> peers_;
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

		uint64_t log_start_index_ = 0;

		std::string vote_for_;
		std::string leader_id_;
		std::size_t election_timeout_ = 3000;
		uint64_t election_timer_id_ = 0;

		int max_pipeline_req = 10;
		
		raft_log log_;

		struct wait_for_commit
		{
			uint64_t index_;
			std::set<std::string> peer_replicated_;
		};
		std::list<wait_for_commit> wait_for_commits_;

		std::string snapshot_filepath_;
		std::ofstream snapshot_;
		raft::snapshot_info snapshot_info_;
	};
	

	inline bool operator ==(const snapshot_info &left, const snapshot_info &right)
	{
		return (left.last_included_term() != right.last_included_term()) ||
			(left.last_snapshot_index() != right.last_snapshot_index());
	}
	inline bool operator !=(const snapshot_info &left, const snapshot_info &right)
	{
		return left == right ? false : true;
	}
}
}