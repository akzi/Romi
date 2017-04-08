#include "nameserver.h"
#include <random>
#include <filesystem>

namespace romi
{
	namespace nameserver
	{
		node::node(nameserver_config cfg)
			:config_(cfg)
		{

		}

		node::~node()
		{

		}

		void node::regist_message()
		{
			REGIST_RECEIVE(sys::net_connect_notify);
			REGIST_RECEIVE(sys::actor_close);

			REGIST_RECEIVE(get_engine_list_req);
			REGIST_RECEIVE(regist_engine_req);
			REGIST_RECEIVE(find_actor_req);
			REGIST_RECEIVE(regist_actor_req);
			REGIST_RECEIVE(write_snapshot_done);

		}

		void node::init()
		{
			regist_message();
			init_node(config_.raft_node_cfg_);
			snapshot_path_ = config_.snapshot_dir_;
		}


		void node::receive(const addr &, const sys::net_connect_notify &notify)
		{
			std::cout << "connect to " << notify.net_connect().remote_addr() << std::endl;
		}

		void node::receive(const addr &from, const get_engine_list_req&)
		{
			get_engine_list_resp resp;
			if (!is_leader())
			{
				resp.set_result(e_no_leader);
				return send(from, resp);
			}
			get_engine_list(resp);

			send(from, resp);
		}

		void node::receive(const addr &from, const find_actor_req &req)
		{
			actor_info info;
			find_actor_resp resp;
			if (!is_leader())
			{
				resp.set_result(e_no_leader);
				return send(from, resp);
			}

			resp.set_result(e_false);
			if (find_actor(req.name(), info))
				resp.set_result(e_true);
			*resp.mutable_actor_info() = info;
			*resp.mutable_req() = req;
			send(from, resp);
		}

		void node::receive(const addr &from, const regist_engine_req &req)
		{
			regist_engine_resp resp;
			auto info = req.engine_info();

			if (!is_leader())
			{
				resp.set_result(e_no_leader);
				return send(from, resp);
			}
			replicate(req.SerializeAsString(), [=](bool status) mutable
			{
				if (!status)
				{
					resp.set_result(e_no_leader);
					return send(from, resp);
				}

				resp.set_result(e_true);

				if (info.engine_id() == 0)
				{
					auto engine_id = unique_id();
					info.set_engine_id(engine_id);
					resp.set_engine_id(engine_id);
				}
				sys::net_connect net_connect;
				net_connect.set_engine_id(info.engine_id());
				net_connect.set_remote_addr(req.engine_info().net_addr());
				connect(net_connect);

				regist_engine(info);

				addr to = from;
				to.set_engine_id(resp.engine_id());
				send(to, resp);
			});
		}

		void node::receive(const addr &from, const regist_actor_req &req)
		{
			regist_actor_resp resp;
			if (!is_leader())
			{
				resp.set_result(e_no_leader);
				return send(from, resp);
			}

			auto info = req.actor_info();
			if (!find_actor(info.name(), info))
			{
				regist_actor(info);
				resp.set_result(e_true);
			}
			watch(from);
			send(from, resp);
		}

		void node::receive(const addr &from, const sys::actor_close &msg)
		{
			unregist_actor(msg.addr());
		}

		void node::receive(const addr &from, const write_snapshot_done &req)
		{
			if (req.ok())
			{

			}
		}


		void node::get_engine_list(get_engine_list_resp &resp)
		{
			for (auto &itr : engines_)
			{
				resp.add_engine_info()->CopyFrom(itr.second);
			}
		}

		void node::regist_actor(const actor_info & info)
		{
			actors_[info.name()] = info;
		}


		void node::unregist_actor(const addr& _addr)
		{
			for (auto itr = actors_.begin(); itr != actors_.end(); ++itr)
			{
				if (itr->second.addr() == _addr)
				{
					actors_.erase(itr);
					return;
				}
			}
		}

		bool node::find_actor(const std::string & name, actor_info &info)
		{
			auto itr = actors_.find(name);
			if (itr != actors_.end())
			{
				info = itr->second;
				return true;
			}
			return false;
		}

		void node::regist_engine(const engine_info &engine)
		{
			engines_[engine.engine_name()] = engine;
		}

		bool node::find_engine(const std::string &name, engine_info &engine)
		{
			auto itr = engines_.find(name);
			if (itr != engines_.end())
			{
				engine = itr->second;
				return true;
			}
			return false;
		}

		bool node::find_engine(uint64_t id, engine_info &engine)
		{
			for (auto &itr : engines_)
			{
				if (id == itr.second.engine_id())
					engine = itr.second;
				return true;
			}
			return false;
		}

		uint64_t node::unique_id()
		{
			next_engine_id_++;
			return std::chrono::high_resolution_clock::now()
				.time_since_epoch().count() + next_engine_id_;
		}

		void node::repicate_callback(const std::string & data, uint64_t index)
		{

		}

		std::string node::get_snapshot_file(uint64_t index)
		{
			return{};
		}

		void node::receive_snapshot_callback(raft::snapshot_info info, std::string &filepath)
		{
			filepath = make_snapshot_name(info);
		}

		void node::receive_snashot_file_failed(std::string &filepath)
		{

		}

		void node::receive_snashot_file_success(std::string &filepath)
		{

		}

		bool node::support_snapshot()
		{
			return true;
		}

		void node::make_snapshot_callback(raft::snapshot_info info)
		{
			std::map<std::string, engine_info> engines = engines_;
			std::map<std::string, actor_info> actors = actors_;
			uint64_t next_engine_id = next_engine_id_;
			std::string filename = make_snapshot_name(info);

			add_job([=] {

			});
		}

		std::string node::make_snapshot_name(raft::snapshot_info info)
		{
			std::string filepath = snapshot_path_;
			if (filepath.back() != '\\' && filepath.back() != '/')
				filepath.push_back('/');

			filepath += std::to_string(info.last_included_term());
			filepath += ".snapshot__temp";
		}

	}
	

}

