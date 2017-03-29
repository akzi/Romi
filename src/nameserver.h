#pragma once
namespace romi
{
	class nameserver
	{
	public:
		nameserver();

		~nameserver();

		void regist_actor(const actor_info & info);

		void regist_engine(engine_info &engine);

		bool find_actor(const std::string & name, addr &addr_);

		bool find_engine(uint64_t id, engine_info &engine);

		bool find_engine(const std::string &name, engine_info &engine);
	private:

		struct  engine_map
		{
			std::mutex mutex_;
			std::map<std::string, engine_info> engine_map_;
		} engine_map_;
		
		struct actor_map 
		{
			std::mutex mutex_;
			std::map<std::string, actor_info> actor_names_;
		} actor_map_;
		
	};
}