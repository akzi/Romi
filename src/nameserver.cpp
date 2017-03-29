#include "romi.hpp"

namespace romi
{

	nameserver::~nameserver()
	{

	}

	void nameserver::regist_actor(const actor_info & info)
	{
		std::lock_guard<std::mutex> locker(actor_map_.mutex_);
		actor_map_.actor_names_[info.name()] = info;
	}

	bool nameserver::find_actor(const std::string & name, addr &addr_)
	{
		std::lock_guard<std::mutex> locker(actor_map_.mutex_);
		auto itr = actor_map_.actor_names_.find(name);
		if (itr != actor_map_.actor_names_.end())
		{
			addr_ = itr->second.addr();
			return true;
		}
		return false;
	}

	void nameserver::regist_engine(engine_info &engine)
	{
		std::lock_guard<std::mutex> locker(engine_map_.mutex_);
		engine_map_.engine_map_[engine.name()] = engine;
	}

	bool nameserver::find_engine(const std::string &name, engine_info &engine)
	{
		std::lock_guard<std::mutex> locker(engine_map_.mutex_);
		auto itr = engine_map_.engine_map_.find(name);
		if (itr != engine_map_.engine_map_.end())
		{
			engine = itr->second;
			return true;
		}
		return false;
	}

	bool nameserver::find_engine(uint64_t id, engine_info &engine)
	{
		std::lock_guard<std::mutex> locker(engine_map_.mutex_);
		for (auto &itr : engine_map_.engine_map_)
		{
			if (id == itr.second.engine_id())
				engine = itr.second;
			return true;
		}
		return false;
	}

	nameserver::nameserver()
	{

	}

}

