#include <random>
#include "romi.hpp"
#include "raft.pb.h"
#include "raft_log.h"
#include "node.h"

namespace romi
{
	namespace raft
	{

		node::node()
		{
			REGIST_RECEIVE(raft::vote_request);
			REGIST_RECEIVE(raft::vote_response);
			REGIST_RECEIVE(raft::replicate_log_entries_request);
			REGIST_RECEIVE(raft::replicate_log_entries_response);
			REGIST_RECEIVE(raft::install_snapshot_request);
			REGIST_RECEIVE(raft::install_snapshot_response);
		}

		void node::connect_node()
		{
			for (auto itr : peers_)
			{
				sys::net_connect connect_;
				connect_.set_engine_id(itr.addr_.engine_id());
				connect_.set_remote_addr(itr.net_addr_);
				connect(connect_);
			}
		}

		void node::set_election_timer()
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(1, (int)election_timeout_);
			election_timer_id_
				= set_timer(dis(gen), [this]
			{
				if (!election_timer_id_)
					return false;
				do_election();
				set_election_timer();
				return false;
			});
		}

		void node::do_election()
		{
			current_term_++;
			vote_requests_.clear();
			for (auto &itr : peers_)
			{
				raft::vote_request req;
				req.set_req_id(gen_req_id());
				req.set_candidate(raft_id_);
				req.set_term(current_term_);
				req.set_last_log_index(last_log_index_);
				req.set_last_log_term(last_log_term_);
				vote_requests_.emplace(req.req_id(), req);
				send(itr.addr_, req);
			}
		}
		void node::cancel_election_timer()
		{
			if (election_timer_id_)
				cancel_timer(election_timer_id_);
			election_timer_id_ = 0;
		}
		void node::set_down(uint64_t term)
		{
			if (current_term_ < term)
			{
				current_term_ = term;
				leader_id_.clear();
				vote_for_.clear();
				/*
				if(snapshot_writer_)
				snapshot_writer_.discard();
				*/
			}
			if (state_ == e_candidate)
			{
				vote_responses_.clear();
				cancel_election_timer();
			}
			if (state_ == e_leader)
			{
				//
			}
			state_ = e_follower;
			//notify_noleader_error();
			set_election_timer();
		}

		void node::become_leader()
		{
			state_ = e_leader;
			cancel_election_timer();
			replicate_log_entry();
		}
		void node::replicate_log_entry()
		{
			for (auto itr : peers_)
			{
				raft::replicate_log_entries_request req;
				req.set_req_id(gen_req_id());
				req.set_leader_commit(committed_index_);
				req.set_leader_id(raft_id_);
				req.set_term(current_term_);
				add_log_entries(itr.next_index_, itr.next_index_ == 0 ? 1:0, req);
				send(itr.addr_, req);
			}
		}

		void node::add_log_entries(uint64_t next_index, 
			uint32_t max_bytes, raft::replicate_log_entries_request &req)
		{
			log_.fill_log_entries(raft_id_, next_index, max_bytes, req);
		}

		uint64_t node::gen_req_id()
		{
			return ++req_id_;
		}

		int node::majority()
		{
			return (int)(peers_.size() + 1) / 2 + 1;
		}

		void node::init_node()
		{
			connect_node();
		}

		void node::receive(const addr &from, 
			const romi::raft::vote_request &req)
		{
			raft::vote_response resp;
			resp.set_log_ok(true);

			if (req.last_log_term() < last_log_term_)
			{
				resp.set_log_ok(false);
			}
			else if (req.last_log_term() == last_log_term_)
			{
				if (req.last_log_index() < last_log_index_)
					resp.set_log_ok(false);
			}

			if (req.term() > current_term_)
			{
				set_down(req.term());
			}

			if (req.term() == current_term_)
			{
				if (resp.log_ok())
				{
					if (vote_for_.empty())
					{
						set_down(req.term());
						vote_for_ = req.candidate();
						resp.set_vote_granted(true);
					}
				}
			}
			resp.set_term(current_term_);
			send(from, resp);
		}

		void node::receive(const addr &from, 
			const raft::vote_response& resp)
		{
			if (state_ != e_candidate)
				return;

			if (resp.term() < current_term_)
				return;

			if (vote_requests_.find(resp.req_id())
				== vote_requests_.end())
				return;

			if (current_term_ < resp.term())
				set_down(resp.term());

			vote_responses_.push_back(resp);
			int votes = 1;

			for (auto &itr : vote_responses_)
			{
				if (itr.vote_granted())
				{
					votes++;
				}
			}

			if (votes >= majority())
			{
				vote_responses_.clear();
				become_leader();
			}
		}

		void node::receive(const addr &from, 
			const raft::install_snapshot_response &resp)
		{

		}

		void node::receive(const addr &from, 
			const raft::install_snapshot_request &resp)
		{

		}

		void node::receive(const addr &from, 
			const raft::replicate_log_entries_request &resp)
		{

		}

		void node::receive(const addr &from, 
			const raft::replicate_log_entries_response &req)
		{

		}

	}
}

