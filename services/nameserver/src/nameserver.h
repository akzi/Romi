#pragma once
#include "romi.hpp"
#include "romi.raft.pb.h"
#include "node.h"
#include "nameserver_config.hpp"

namespace romi
{
	namespace nameserver
	{
		class node :public raft::node
		{
		public:
			node(nameserver_config cfg);

			~node();

		private:
			virtual void init() override;

			void receive(const addr &from, const sys::net_connect_notify &notify);

			void receive(const addr &from, const sys::actor_close &msg);

			void receive(const addr &from, const get_engine_id_req &req);

			void receive(const addr &from, const regist_actor_req &req);

			void receive(const addr &from, const regist_engine_req &req);

			void receive(const addr &from, const find_actor_req &req);

			void receive(const addr &from, const get_engine_list_req &req);

			// msg from write snapshot file threadpool.
			void receive(const addr &from, const write_snapshot_done &req);

			void do_regist_actor(const actor_info & info);

			void unregist_actor(const addr& _addr);

			void do_regist_engine(const engine_info &info);

			void connect_engine(const engine_info &info);

			bool find_actor(const std::string & name, actor_info &info);

			bool find_engine(uint64_t id, engine_info &engine);

			bool find_engine(const std::string &name, engine_info &info);

			void get_engine_list(get_engine_list_resp &resp);

			uint64_t gen_engine_id();
			
			// raft::node
			virtual void repicate_callback(const std::string & data, uint64_t index) override;

			virtual std::string get_snapshot_file(uint64_t index) override;

			virtual void make_snapshot_callback(raft::snapshot_info info) override;

			virtual void receive_snapshot_callback(raft::snapshot_info info, std::string &filepath) override;

			virtual void receive_snashot_file_failed(std::string &filepath) override;

			virtual void receive_snashot_file_success(std::string &filepath) override;

			virtual bool support_snapshot() override;

			//
			void reload_snapshot();

			void reload_wal();

			void regist_message();

			uint64_t gen_version();

			void load_snapshot(std::ifstream &file);

			std::string gen_snapshot_filepath(raft::snapshot_info info);

			void write_wal(uint8_t type, const std::string &data);
			
			void load_wal_file(std::ifstream &file);
		private:

			void reset();


			uint64_t next_engine_id_ = 0;

			std::map<std::string, engine_info> engines_;
			std::map<std::string, actor_info> actors_;

			struct watchers 
			{
				std::set<addr,addr_less> watchers_;
			};
			std::map<std::string, watchers > watchers_list_;

			nameserver_config config_;

			bool building_snapshot_ = false;
			
			std::string snapshot_file_;
			raft::snapshot_info snapshot_info_;


			std::string snapshot_path_;

			std::string wal_path_;
			std::ofstream wal_;

			uint64_t version_ = 0;
		};
	}
}