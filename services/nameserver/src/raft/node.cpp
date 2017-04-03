#include <random>
#include "romi.hpp"
#include "raft.pb.h"
#include "raft_log.h"
#include "node.h"
#include <assert.h>

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
		void node::repicate_callback(const std::string & data, uint64_t index)
		{
			assert(false);
		}

		void node::commit_callback(uint64_t index)
		{
			assert(false);
		}


		void node::no_leader_callback()
		{
			assert(false);
		}

		bool node::is_leader()
		{
			return state_ == e_leader;
		}

		bool node::support_snapshot()
		{
			assert(false);
			return false;
		}

		std::string node::get_snapshot_file(uint64_t index)
		{
			assert(false);
			return{};
		}

		void node::make_snapshot_callback(uint64_t last_include_term, uint64_t last_include_index)
		{
			assert(false);
		}

		uint64_t node::replicate(const std::string &msg)
		{
			assert(is_leader());
			raft::log_entry entry;

			entry.set_index(++last_log_index_);
			entry.set_term(last_log_term_);
			entry.set_log_data_(msg);
			entry.set_type(e_raft_log);

			write_raft_log(entry);
			wait_for_commits_.push_back({ last_log_index_,{} });
			replicate_log_entry();
			return last_log_index_;
		}

		void node::connect_node()
		{
			for (auto itr : peers_)
			{
				sys::net_connect connect_;
				connect_.set_engine_id(itr.second.addr_.engine_id());
				connect_.set_remote_addr(itr.second.net_addr_);
				connect(connect_);
			}
		}

		void node::set_election_timer()
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(1, (int)election_timeout_);
			election_uint64_t_
				= set_timer(dis(gen), [this]
			{
				if (!election_uint64_t_)
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
				send(itr.second.addr_, req);
			}
		}
		void node::cancel_election_timer()
		{
			if (election_uint64_t_)
				cancel_timer(election_uint64_t_);
			election_uint64_t_ = 0;
		}
		void node::set_down(uint64_t term)
		{
			if (current_term_ < term)
			{
				current_term_ = term;
				leader_id_.clear();
				vote_for_.clear();
			}
			if (state_ == e_candidate)
			{
				vote_responses_.clear();
				cancel_election_timer();
			}
			else if (state_ == e_leader)
			{
				for (auto &itr: peers_)
				{
					itr.second.req_ids_.clear();
					if (itr.second.heartbeat_uint64_t_)
					{
						cancel_timer(itr.second.heartbeat_uint64_t_);
						itr.second.heartbeat_uint64_t_ = 0;
						itr.second.match_index_ = 0;
						itr.second.next_index_ = 0;
						itr.second.last_heartbeat_time_ = 
							high_resolution_clock::now();
					}
				}
				wait_for_commits_.clear();
				no_leader_callback();
			}
			state_ = e_follower;
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
				replicate_log_entry(itr.second);
			}
		}

		void node::replicate_log_entry(peer &_peer)
		{
			if (_peer.req_ids_.size() > max_pipeline_req)
				return;
			raft::replicate_log_entries_request req;
			uint32_t max_bytes = _peer.next_index_ == 0 ? 1 : 0;

			add_log_entries(_peer.next_index_, max_bytes, req);

			if (!req.entries_size()  && 
				_peer.next_index_ < last_log_index_)
			{
				if (install_snapshot(_peer))
				{
					if (_peer.heartbeat_uint64_t_)
					{
						set_heartbeat_timer(_peer);
						return;
					}
				}
			}
			auto req_id = gen_req_id();
			req.set_req_id(req_id);
			req.set_leader_commit(committed_index_);
			req.set_leader_id(raft_id_);
			req.set_term(current_term_);
			_peer.req_ids_.insert(req_id);
			send(_peer.addr_, req);
			_peer.last_heartbeat_time_ = high_resolution_clock::now();

			set_heartbeat_timer(_peer);
		}
		bool node::install_snapshot(peer & _peer)
		{
			if (!support_snapshot())
				return false;
			auto path = get_snapshot_file(_peer.next_index_);
			if (path.empty())
			{
				make_snapshot_callback(last_log_term_, last_log_index_);
				return false;
			}
		}

		void node::set_heartbeat_timer(peer &_peer)
		{
			if (_peer.heartbeat_uint64_t_)
				cancel_timer(_peer.heartbeat_uint64_t_);

			auto _addr = _peer.addr_;
			auto diff = high_resolution_clock::now() - 
				_peer.last_heartbeat_time_;

			auto millis = std::chrono::duration_cast<
				std::chrono::milliseconds>(diff).count();

			_peer.heartbeat_uint64_t_ = set_timer(millis, [_addr, this]
			{
				if (state_ != e_leader)
					return false;
				auto itr = peers_.find(_addr);
				if (itr == peers_.end())
					return false;
				itr->second.heartbeat_uint64_t_ = 0;
				replicate_log_entry(itr->second);
				return false;
			});
		}

		void node::add_log_entries(uint64_t next_index,
			uint32_t max_bytes, raft::replicate_log_entries_request &req)
		{
			log_.fill_log_entries(raft_id_, next_index, max_bytes, req);
		}

		romi::raft::log_entry node::get_log_entry(uint64_t index)
		{
			return log_.get_log_entry(raft_id_, index);
		}

		void node::receive(const addr &from,
			const raft::replicate_log_entries_request &req)
		{
			

			raft::replicate_log_entries_response resp;
			resp.set_success(false);
			resp.set_term(current_term_);

			if (current_term_ < req.term())
			{
				set_down(req.term());
				resp.set_term(current_term_);
			}

			leader_id_ = req.leader_id();

			if (last_snapshot_index_ > last_log_index_)
			{
				if (req.prev_log_index() != last_snapshot_index_ || 
					req.prev_log_term() != last_snapshot_term_)
				{
					resp.set_last_log_index(last_snapshot_index_);
					return response(from, resp);
				}
			}
			else if (req.prev_log_index() > last_log_index_)
			{
				resp.set_last_log_index(last_log_index_);
				return response(from, resp);
			}
			else if (req.prev_log_index() != last_snapshot_index_)
			{
				if (req.prev_log_index() > log_start_index_)
				{
					auto entry = get_log_entry(req.prev_log_index());
					if (entry.term() != req.prev_log_term())
					{
						resp.set_last_log_index(req.prev_log_index() - 1);
						return response(from, resp);
					}
				}
			}

			resp.set_success(true);
			auto check_log = true;
			std::list<std::pair<uint64_t, std::string>> entries;
			for (int i = 0; i < req.entries_size(); ++i)
			{
				auto entry = req.entries(i);
				if (check_log)
				{
					if (entry.index() < log_start_index_)
						continue;
					if (entry.index() <= last_log_index_)
					{
						if (get_log_entry(entry.index()).term()== entry.term())
							continue;
						assert(committed_index_ < entry.index());
						log_truncate_suffix(entry.index());
						check_log = false;
					}
				}
				entries.emplace_back(entry.index(),entry.SerializeAsString());
			}

			write_raft_log(entries);

			resp.set_last_log_index(last_log_index_);
			response(from, resp);

			//commit 
			if (committed_index_ < req.leader_commit())
			{
				auto entries = get_log_entries(committed_index_ + 1, 
					req.leader_commit() - committed_index_);

				for (auto &itr : entries)
				{
					assert(itr.index() == committed_index_ + 1);
					repicate_callback(itr.log_data_(), itr.index());
					committed_index_ = itr.index();
				}
			}
		}

		void node::receive(const addr &from,
			const raft::replicate_log_entries_response &resp)
		{
			if (state_ != e_leader || 
				peers_.find(from) == peers_.end())
				return;

			auto &peer = peers_[from];
			if (peer.req_ids_.find(resp.req_id()) == peer.req_ids_.end())
				return;

			peer.req_ids_.erase(resp.req_id());
			peer.last_heartbeat_time_ = high_resolution_clock::now();
			if (!resp.success())
			{
				peer.match_index_ = 0;
				if (current_term_ < resp.term())
				{
					set_down(resp.term());
					return;
				}
				peer.next_index_ = resp.last_log_index() + 1;
				return replicate_log_entry(peer);
			}

			peer.match_index_ = resp.last_log_index();
			peer.next_index_ = peer.match_index_ + 1;

			if (peer.match_index_ <= committed_index_)
				return replicate_log_entry(peer);

			check_commit_log_entries(peer.raft_id_, peer.match_index_);
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

		uint64_t node::gen_req_id()
		{
			return ++req_id_;
		}

		void node::response(const addr &from, 
			const raft::replicate_log_entries_response &resp)
		{
			set_election_timer();
			send(from, resp);
		}

		int node::majority()
		{
			return (int)(peers_.size() + 1) / 2 + 1;
		}

		void node::log_truncate_suffix(uint64_t index)
		{

		}

		void node::write_raft_log(
			const std::list<std::pair<uint64_t, std::string>> &entries)
		{
			log_.write_raft_log(raft_id_, entries);
			last_log_index_ = entries.back().first;
		}

		void node::write_raft_log(const raft::log_entry &entry)
		{
			log_.write_raft_log(raft_id_, 
				std::make_pair(entry.index(), entry.SerializeAsString()));
		}

		std::list<raft::log_entry>
			node::get_log_entries(uint64_t index, uint64_t count)
		{
			std::list<std::pair<uint64_t, std::string>> entries;
			std::list<raft::log_entry> log_entries;
			if (!log_.get_log_entrys(raft_id_, index, (uint32_t)count, entries))
				throw std::runtime_error("log get_log_entrire fail");

			for (auto &itr: entries)
			{
				raft::log_entry _log_entry;
				_log_entry.ParseFromString(itr.second);
				log_entries.push_back(_log_entry);
			}
			return log_entries;
		}

		void node::init_node()
		{
			connect_node();
		}

		void node::check_commit_log_entries(const std::string &raft_id, uint64_t index)
		{
			for (auto itr = wait_for_commits_.begin(); itr != wait_for_commits_.end(); )
			{
				if (itr->index_ <= index)
				{
					itr->peer_replicated_.insert(raft_id);
					if (itr->peer_replicated_.size() > majority())
					{
						commit_callback(itr->index_);
						itr = wait_for_commits_.erase(itr);
						continue;
					}
				}
				++itr;
			}
		}

	}
}

