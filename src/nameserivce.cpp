#include "nameservice.h"

namespace romi
{
	nameserivce::nameserivce()
	{

	}

	nameserivce::~nameserivce()
	{

	}

	void nameserivce::regist_actor(const actor_info & info)
	{
		actor_map_.actor_names_[info.name()] = info;
	}

	bool nameserivce::find_actor(const std::string & name, addr &addr_)
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

	void nameserivce::regist_engine(engine_info &engine)
	{
		std::lock_guard<std::mutex> locker(engine_map_.mutex_);
		engine_map_.engine_map_[engine.name()] = engine;
	}

	bool nameserivce::find_engine(const std::string &name, engine_info &engine)
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

	bool nameserivce::find_engine(uint64_t id, engine_info &engine)
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



}

