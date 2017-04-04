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
		init_node();
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

	void nameserver::init_node()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void nameserver::repicate_callback(const std::string & data, uint64_t index)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void nameserver::commit_callback(uint64_t index)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void nameserver::no_leader_callback()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	std::string nameserver::get_snapshot_file(uint64_t index)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void nameserver::make_snapshot_callback(uint64_t last_include_term, uint64_t last_include_index)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void nameserver::new_snapshot_callback(raft::snapshot_info info, std::string &filepath)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void nameserver::receive_snashot_file_failed(std::string &filepath)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void nameserver::receive_snashot_file_success(std::string &filepath)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	bool nameserver::support_snapshot()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

}

