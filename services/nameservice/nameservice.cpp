#include "romi.hpp"
#include "nameservice.h"

namespace romi
{
	nameservice::nameservice()
	{

	}

	nameservice::~nameservice()
	{

	}

	void nameservice::regist_actor(const actor_info & info)
	{
		actor_names_[info.name()] = info;
	}


	void nameservice::unregist_actor(const addr& _addr)
	{
		for(auto itr = actor_names_.begin(); itr != actor_names_.end(); ++itr)
		{
			if (itr->second.addr() == _addr)
			{
				actor_names_.erase(itr);
				return;
			}
		}
	}

	bool nameservice::find_actor(const std::string & name, actor_info &info)
	{
		auto itr = actor_names_.find(name);
		if (itr != actor_names_.end())
		{
			info = itr->second;
			return true;
		}
		return false;
	}

	void nameservice::regist_engine(const engine_info &engine)
	{
		engine_map_[engine.name()] = engine;
	}

	bool nameservice::find_engine(const std::string &name, engine_info &engine)
	{
		auto itr = engine_map_.find(name);
		if (itr != engine_map_.end())
		{
			engine = itr->second;
			return true;
		}
		return false;
	}

	bool nameservice::find_engine(uint64_t id, engine_info &engine)
	{
		for (auto &itr : engine_map_)
		{
			if (id == itr.second.engine_id())
				engine = itr.second;
			return true;
		}
		return false;
	}

	uint64_t nameservice::unique_id()
	{
		next_engine_id_++;
		return std::chrono::high_resolution_clock::now()
			.time_since_epoch().count() + next_engine_id_;
	}

	void nameservice::init()
	{
		std::cout << "nameservice init." << std::endl;

		receivce([this](const addr &, const sys::net_connect_notify &notify) {

			std::cout << "connect to "<<notify.net_connect().remote_addr() << std::endl;
		});

		receivce([this](const addr &from, const sys::actor_close &msg) {

			unregist_actor(msg.addr());
		});

		receivce([this](const addr &from, const sys::regist_actor_req &req) {
		
			sys::regist_actor_resp resp;
			auto info = req.actor_info();
			if (!find_actor(info.name(), info))
			{
				regist_actor(info);
				resp.set_result(sys::regist_actor_result::OK);
			}
			watch(from);
			send(from, resp);
		});

		receivce([this](const addr &from, const sys::regist_engine_req &req) {

			sys::regist_engine_resp resp;
			auto info = req.engine_info();

			resp.set_result(true);

			if (info.engine_id() == 0)
			{
				auto engine_id =  unique_id();
				info.set_engine_id(engine_id);
				resp.set_engine_id(engine_id);
			}
			connect_engine(info);
			regist_engine(info);

			addr to = from;
			to.set_engine_id(resp.engine_id());
			send(to, resp);
		});

		receivce([this](const addr &from, const sys::find_actor_req &req) {

			actor_info info;
			sys::find_actor_resp resp;
			
			resp.set_find(false);
			if (find_actor(req.name(), info))
				resp.set_find(true);
			*resp.mutable_actor_info() = info;
			*resp.mutable_req() = req;
			send(from, resp);
		});

		receivce([this](const addr &from, const sys::get_engine_list_req&) {

			sys::get_engine_list_resp resp;
			get_engine_list(resp);

			send(from, resp);
		});
	}

	void nameservice::connect_engine(const ::romi::engine_info& engine_info)
	{
		sys::net_connect msg;
		msg.mutable_from()->CopyFrom(get_addr());
		msg.mutable_remote_addr()->append(engine_info.addr());
		msg.set_engine_id(engine_info.engine_id());

		connect(msg);
	}

	void nameservice::get_engine_list(sys::get_engine_list_resp &resp)
	{
		for (auto &itr : engine_map_)
		{
			resp.add_engine_info()->CopyFrom(itr.second);
		}
	}

}

