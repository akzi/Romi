#pragma once
namespace romi
{
	class nameservice :public actor
	{
	public:
		nameservice();

		~nameservice();

	private:

		virtual void init() override;

		void regist_actor(const actor_info & info);

		void unregist_actor(const addr& _addr);

		void regist_engine(const engine_info &engine);

		bool find_actor(const std::string & name, actor_info &info);

		bool find_engine(uint64_t id, engine_info &engine);

		bool find_engine(const std::string &name, engine_info &engine);

		uint64_t unique_id()
		{
			next_engine_id_++;
			return std::chrono::high_resolution_clock::now().time_since_epoch().count() + next_engine_id_;
		}

		uint64_t next_engine_id_ = 0;

		std::map<std::string, engine_info> engine_map_;
		std::map<std::string, actor_info> actor_names_;
	};
}