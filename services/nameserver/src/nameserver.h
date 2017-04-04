#pragma once
#include "romi.hpp"
#include "raft.pb.h"
#include "raft/node.h"
#include "nameserver_config.hpp"

namespace romi
{
	
	class nameserver :public raft::node
	{
	public:
		nameserver(nameserver_config cfg);

		~nameserver();

	private:

		virtual void init() override;

		void regist_message();
		//sys
		void receive(const addr &from, const sys::net_connect_notify &notify);

		void receive(const addr &from, const sys::actor_close &msg);
		
		void receive(const addr &from, const sys::regist_actor_req &req);

		void receive(const addr &from, const sys::regist_engine_req &req);

		void receive(const addr &from, const sys::find_actor_req &req);

		void receive(const addr &from, const sys::get_engine_list_req &req);

		//
		void regist_actor(const actor_info & info);

		void unregist_actor(const addr& _addr);

		void regist_engine(const engine_info &engine);

		bool find_actor(const std::string & name, actor_info &info);

		bool find_engine(uint64_t id, engine_info &engine);

		bool find_engine(const std::string &name, engine_info &engine);

		void connect_engine(const ::romi::engine_info& engine_info);

		void get_engine_list(sys::get_engine_list_resp &resp);

		uint64_t unique_id();
		
		//from raft
		virtual void init_node() override;

		virtual void repicate_callback(const std::string & data, uint64_t index) override;

		virtual void commit_callback(uint64_t index) override;

		virtual void no_leader_callback() override;

		virtual std::string get_snapshot_file(uint64_t index) override;

		virtual void make_snapshot_callback(uint64_t last_include_term, uint64_t last_include_index) override;

		virtual void new_snapshot_callback(raft::snapshot_info info, std::string &filepath) override;

		virtual void receive_snashot_file_failed(std::string &filepath) override;

		virtual void receive_snashot_file_success(std::string &filepath) override;

		virtual bool support_snapshot() override;

		uint64_t next_engine_id_ = 0;

		std::map<std::string, engine_info> engine_map_;
		std::map<std::string, actor_info> actor_names_;

		nameserver_config config_;

		uint64_t req_id_ = 1;
	};
}