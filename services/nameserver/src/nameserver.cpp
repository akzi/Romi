#include "nameserver.h"
#include <random>

namespace romi
{
	nameserver::nameserver(nameserver_config cfg)
		:config_(cfg)
	{

	}

	nameserver::~nameserver()
	{

	}

	void nameserver::regist_message()
	{
		REGIST_RECEIVE(raft::vote_request);
		REGIST_RECEIVE(raft::vote_response);
		REGIST_RECEIVE(raft::append_entries_request);
		REGIST_RECEIVE(raft::append_entries_response);
		REGIST_RECEIVE(raft::install_snapshot_request);
		REGIST_RECEIVE(raft::install_snapshot_response);

		REGIST_RECEIVE(sys::net_connect_notify);
		REGIST_RECEIVE(sys::get_engine_list_req);
		REGIST_RECEIVE(sys::regist_engine_req);
		REGIST_RECEIVE(sys::find_actor_req);
		REGIST_RECEIVE(sys::regist_actor_req);
		REGIST_RECEIVE(sys::actor_close);
	}

	void nameserver::init()
	{
		regist_message();
		connect_node();
		set_election_timer();
	}

	void nameserver::connect_node()
	{
		for (auto itr: config_.nameserver_cluster_)
		{
			sys::net_connect connect_;
			connect_.set_engine_id(itr.engine_id_);
			connect_.set_remote_addr(itr.addr_);
			connect(connect_);
		}
	}

	void nameserver::set_election_timer()
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(1, (int)raft_info_.election_timeout_);
		raft_info_.election_timer_id_ 
			= set_timer(dis(gen), [this] 
		{
			if (!raft_info_.election_timer_id_)
				return false;
			do_election();
			set_election_timer();
			return false;
		});
	}

	void nameserver::do_election()
	{
		raft_info_.current_term_++;
		raft_info_.vote_requests_.clear();
		for (auto &itr:raft_info_.cluster_)
		{
			raft::vote_request req;
			req.set_req_id(gen_req_id());
			req.set_candidate(raft_info_.raft_id_);
			req.set_term(raft_info_.current_term_);
			req.set_last_log_index(raft_info_.last_log_index_);
			req.set_last_log_term(raft_info_.last_log_term_);
			raft_info_.vote_requests_.emplace(req.req_id(), req);
			send(itr.addr_, req);
		}
	}
	void nameserver::cancel_election_timer()
	{
		if(raft_info_.election_timer_id_)
			cancel_timer(raft_info_.election_timer_id_);
		raft_info_.election_timer_id_ = 0;
	}
	void nameserver::set_down(uint64_t term)
	{
		if (raft_info_.current_term_ < term)
		{
			raft_info_.current_term_ = term;
			raft_info_.leader_id_.clear();
			raft_info_.vote_for_.clear();
			/*
			if(snapshot_writer_)
					snapshot_writer_.discard();
			*/
		}
		if (raft_info_.state_ == raft::state::e_candidate)
		{
			raft_info_.vote_responses_.clear();
			cancel_election_timer();
		}
		if (raft_info_.state_ == raft::state::e_leader)
		{
			//
		}
		raft_info_.state_ = raft::state::e_follower;
		//notify_noleader_error();
		set_election_timer();
	}

	void nameserver::become_leader()
	{
		raft_info_.state_ = raft::e_leader;
		cancel_election_timer();
		replicate_log_entry();
	}
	void nameserver::replicate_log_entry()
	{
		for (auto itr: raft_info_.cluster_)
		{
			raft::replicate_log_entries_request req;
			req.set_req_id(gen_req_id());
			req.set_leader_commit(raft_info_.committed_index_);
			req.set_leader_id(raft_info_.raft_id_);
			req.set_term(raft_info_.current_term_);
			add_log_entries(req, itr.next_index_);
			send(itr.addr_, req);
		}
	}
	void nameserver::add_log_entries(raft::replicate_log_entries_request &req, uint64_t next_index)
	{
		req.add_entries()->set_index();
	}
	uint64_t nameserver::gen_req_id()
	{
		return ++req_id_;
	}

	void nameserver::receive(const addr &from, const romi::raft::vote_request &req)
	{
		raft::vote_response resp;
		resp.set_log_ok(true);

		if (req.last_log_term() < raft_info_.last_log_term_)
		{
			resp.set_log_ok(false);
		}
		else if (req.last_log_term() == raft_info_.last_log_term_)
		{
			if (req.last_log_index() < raft_info_.last_log_index_)
				resp.set_log_ok(false);
		}

		if (req.term() > raft_info_.current_term_)
		{
			set_down(req.term());
		}

		if (req.term() == raft_info_.current_term_)
		{
			if (resp.log_ok())
			{
				if (raft_info_.vote_for_.empty())
				{
					set_down(req.term());
					raft_info_.vote_for_ = req.candidate();
					resp.set_vote_granted(true);
				}
			}
		}
		resp.set_term(raft_info_.current_term_);
		send(from, resp);
	}

	void nameserver::receive(const addr &from, const raft::vote_response& resp)
	{
		if (raft_info_.state_ != raft::e_candidate)
			return;

		if (resp.term() < raft_info_.current_term_)
			return;

		if (raft_info_.vote_requests_.find(resp.req_id()) 
			== raft_info_.vote_requests_.end())
			return;

		if (raft_info_.current_term_ < resp.term())
			set_down(resp.term());

		raft_info_.vote_responses_.push_back(resp);
		int votes = 1;

		for (auto &itr: raft_info_.vote_responses_)
		{
			if (itr.vote_granted())
			{
				votes++;
			}
		}

		if (votes >= (raft_info_.cluster_.size() + 1) / 2 + 1)
		{
			raft_info_.vote_responses_.clear();
			become_leader();
		}
	}

	void nameserver::receive(const addr &from, const raft::append_entries_request &req)
	{

	}

	void nameserver::receive(const addr &from, const raft::append_entries_response &resp)
	{

	}

	void nameserver::receive(const addr &from, const raft::install_snapshot_request &resp)
	{

	}

	void nameserver::receive(const addr &from, const raft::install_snapshot_response &resp)
	{

	}

	void nameserver::receive(const addr &, const sys::net_connect_notify &notify)
	{
		std::cout << "connect to " << notify.net_connect().remote_addr() << std::endl;
	}

	void nameserver::receive(const addr &from, const sys::get_engine_list_req&)
	{
		sys::get_engine_list_resp resp;
		get_engine_list(resp);

		send(from, resp);
	}

	void nameserver::receive(const addr &from, const sys::find_actor_req &req)
	{
		actor_info info;
		sys::find_actor_resp resp;

		resp.set_find(false);
		if (find_actor(req.name(), info))
			resp.set_find(true);
		*resp.mutable_actor_info() = info;
		*resp.mutable_req() = req;
		send(from, resp);
	}

	void nameserver::receive(const addr &from, const sys::regist_engine_req &req)
	{
		sys::regist_engine_resp resp;
		auto info = req.engine_info();

		resp.set_result(true);

		if (info.engine_id() == 0)
		{
			auto engine_id = unique_id();
			info.set_engine_id(engine_id);
			resp.set_engine_id(engine_id);
		}
		connect_engine(info);
		regist_engine(info);

		addr to = from;
		to.set_engine_id(resp.engine_id());
		send(to, resp);
	}

	void nameserver::receive(const addr &from, const sys::regist_actor_req &req)
	{
		sys::regist_actor_resp resp;
		auto info = req.actor_info();
		if (!find_actor(info.name(), info))
		{
			regist_actor(info);
			resp.set_result(sys::regist_actor_result::OK);
		}
		watch(from);
		send(from, resp);
	}

	void nameserver::receive(const addr &from, const sys::actor_close &msg)
	{
		unregist_actor(msg.addr());
	}


	void nameserver::connect_engine(const romi::engine_info& engine_info)
	{
		sys::net_connect msg;
		msg.mutable_from()->CopyFrom(get_addr());
		msg.mutable_remote_addr()->append(engine_info.addr());
		msg.set_engine_id(engine_info.engine_id());

		connect(msg);
	}

	void nameserver::get_engine_list(sys::get_engine_list_resp &resp)
	{
		for (auto &itr : engine_map_)
		{
			resp.add_engine_info()->CopyFrom(itr.second);
		}
	}

	void nameserver::regist_actor(const actor_info & info)
	{
		actor_names_[info.name()] = info;
	}


	void nameserver::unregist_actor(const addr& _addr)
	{
		for (auto itr = actor_names_.begin(); itr != actor_names_.end(); ++itr)
		{
			if (itr->second.addr() == _addr)
			{
				actor_names_.erase(itr);
				return;
			}
		}
	}

	bool nameserver::find_actor(const std::string & name, actor_info &info)
	{
		auto itr = actor_names_.find(name);
		if (itr != actor_names_.end())
		{
			info = itr->second;
			return true;
		}
		return false;
	}

	void nameserver::regist_engine(const engine_info &engine)
	{
		engine_map_[engine.name()] = engine;
	}

	bool nameserver::find_engine(const std::string &name, engine_info &engine)
	{
		auto itr = engine_map_.find(name);
		if (itr != engine_map_.end())
		{
			engine = itr->second;
			return true;
		}
		return false;
	}

	bool nameserver::find_engine(uint64_t id, engine_info &engine)
	{
		for (auto &itr : engine_map_)
		{
			if (id == itr.second.engine_id())
				engine = itr.second;
			return true;
		}
		return false;
	}

	uint64_t nameserver::unique_id()
	{
		next_engine_id_++;
		return std::chrono::high_resolution_clock::now()
			.time_since_epoch().count() + next_engine_id_;
	}
}

