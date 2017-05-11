#include "nameserver.h"
#include <random>
#include <filesystem>
#include "utils.hpp"

namespace romi
{
	namespace nameserver
	{
		const static std::string g_nameserver_wal= "nameserver_wal";
		const static std::string g_nameserver_snapshot = "nameserver_snapshot";
		const static std::string g_snapshot__temp_ext = ".snapshot__temp";
		const static std::string g_snapshot_ext = ".snapshot";

		enum wal_type:uint8_t
		{
			e_regist_engine = 1,
			e_unregist_engine,
		};
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
			wal_path_ = config_.wal_path_;

			reload_snapshot();
			reload_wal();
		}


		void node::reload_snapshot()
		{
			std::vector<std::string> to_delete;
			std::map<std::string, std::string> snapshots;
			auto files = ls_files()(snapshot_path_);

			for (auto &itr: files)
			{
				if (get_extension_name()(itr) != g_snapshot_ext)
				{
					to_delete.push_back(itr);
				}
				else
				{
					snapshots.emplace(get_filename()(itr),itr);
				}
			}

			if (snapshots.size())
			{
				auto last_snapshot = snapshots.rbegin()->second;
				std::ifstream file;
				file.open(last_snapshot);
				if (!file.good())
				{
					throw std::runtime_error("open file error: " + last_snapshot);
				}
				auto str = decode_string(file);
				if (str != g_nameserver_snapshot)
				{
					throw std::runtime_error("is not snapshot file");
				}
				load_snapshot(file);
				file.close();

				for (auto &itr: snapshots)
				{
					if (itr.second != last_snapshot)
					{
						to_delete.push_back(itr.second);
					}
				}
			}
			//delete useless snapshot files;
			for (auto &itr: to_delete)
			{
				remove(itr.c_str());
			}
		}


		void node::reload_wal()
		{
			std::vector<std::string> to_delete;
			std::map<std::string, std::string> wals;
			auto files = ls_files()(wal_path_);
			
			for (auto itr: files)
			{
				wals.emplace(get_filename()(itr), itr);
			}
			if (wals.size())
			{
				auto last_snapshot = wals.rbegin()->second;
				std::ifstream file;
				file.open(last_snapshot);
				if (!file.good())
				{
					throw std::runtime_error("open file error: " + last_snapshot);
				}
				auto str = decode_string(file);
				if (str != g_nameserver_wal)
				{
					throw std::runtime_error("is not wal file");
				}
				load_wal(file);
				file.close();

				for (auto &itr : wals)
				{
					if (itr.second != last_snapshot)
					{
						to_delete.push_back(itr.second);
					}
				}
			}
			//delete useless snapshot files;
			for (auto &itr : to_delete)
			{
				remove(itr.c_str());
			}
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

			replicate(pack_message(req), [=](bool status) mutable
			{
				if (!status)
				{
					resp.set_result(e_no_leader);
					return send(from, resp);
				}
				resp.set_result(e_true);
				regist_engine(info);
				send(from, resp);
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
				auto name = get_filename()(req.snapshot_path());
				auto new_name = name + g_snapshot_ext;
				rename(req.snapshot_path().c_str(),new_name.c_str());
				remove(snapshot_file_.c_str());
				snapshot_file_ = new_name;
			}
			else
			{
				remove(req.snapshot_path().c_str());
			}
			building_snapshot_ = false;
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

		void node::regist_engine(const engine_info &info)
		{
			sys::net_connect net_connect;
			net_connect.set_engine_id(info.engine_id());
			net_connect.set_remote_addr(info.net_addr());
			connect(net_connect);

			engines_[info.engine_name()] = info;
			write_wal(wal_type::e_regist_engine, info.SerializeAsString());
		}

		bool node::find_engine(const std::string &name, engine_info &info)
		{
			auto itr = engines_.find(name);
			if (itr != engines_.end())
			{
				info = itr->second;
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

		void node::repicate_callback(const std::string &data, uint64_t)
		{
			uint8_t *ptr = (uint8_t*)data.data();

			auto type = decode_string(ptr);

			if (type == get_message_type<regist_engine_req>())
			{
				regist_engine_req req;
				req.ParseFromString(decode_string(ptr));

				regist_engine(req.engine_info());
			}
		}

		std::string node::get_snapshot_file(uint64_t index)
		{
			if (snapshot_info_.last_snapshot_index() >= index)
				return snapshot_file_;
			return{};
		}

		void node::receive_snapshot_callback(raft::snapshot_info info, std::string &filepath)
		{
			filepath = make_snapshot_name(info);
		}

		void node::receive_snashot_file_failed(std::string &filepath)
		{
			remove(filepath.c_str());
		}

		void node::receive_snashot_file_success(std::string &filepath)
		{
			std::ifstream file;
			file.open(filepath);
			if (!file.good())
			{
				std::cout << filepath.c_str() << " open failed" << std::endl;
				return;
			}
			auto str = decode_string(file);
			if (str != g_nameserver_snapshot)
			{
				std::cout << "snapshot file header error" << std::endl;
				return;
			}
			remove(snapshot_file_.c_str());
			snapshot_file_ = filepath;

			load_snapshot(file);
			file.close();
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
			auto send_msg_handle = get_send_msg_handle();
			auto addr_ = get_addr();
			auto version = version_;

			add_job([=] 
			{
				write_snapshot_done snapshot_done;
				std::ofstream file;
				file.open(filename.c_str());
				if (!file.good())
				{
					snapshot_done.set_ok(false);
					send_msg_handle(make_message(addr_, addr_, snapshot_done));
					return;
				}
				encode_string(file, g_nameserver_snapshot);
				encode_uint64(file, version);
				encode_uint64(file, next_engine_id);
				encode_string(file, info.SerializeAsString());


				encode_uint64(file, engines.size());
				for (auto &iter :engines)
				{
					encode_string(file, iter.first);
					encode_string(file, iter.second.SerializeAsString());
				}

				encode_uint64(file, actors.size());
				for (auto &iter : actors)
				{
					encode_string(file, iter.first);
					encode_string(file, iter.second.SerializeAsString());
				}

				file.close();

				snapshot_done.set_ok(true);
				snapshot_done.set_snapshot_path(filename);
				send_msg_handle(make_message(addr_, addr_, snapshot_done));
				return;
			});
		}
		void node::load_snapshot(std::ifstream &file)
		{
			engines_.clear();
			actors_.clear();
			snapshot_info_.Clear();

			version_ = decode_uint64(file);
			next_engine_id_ = decode_uint64(file);
			snapshot_info_.ParseFromString(decode_string(file));

			auto size = decode_uint64(file);
			for (uint64_t i = 0; i < size; i++)
			{
				auto first = decode_string(file);
				engine_info second;
				second.ParseFromString(decode_string(file));
				engines_.emplace(first, second);
			}

			size = decode_uint64(file);
			for (uint64_t i = 0; i < size; i++)
			{
				auto first = decode_string(file);
				actor_info second;
				second.ParseFromString(decode_string(file));
				actors_.emplace(first, second);
			}
		}

		std::string node::make_snapshot_name(raft::snapshot_info info)
		{
			std::string filepath = snapshot_path_;
			if (filepath.back() != '\\' && filepath.back() != '/')
				filepath.push_back('/');

			filepath += std::to_string(info.last_snapshot_index());
			filepath += g_snapshot__temp_ext;

			return filepath;
		}

		uint64_t node::gen_version()
		{
			return ++version_;
		}

		void node::write_wal(uint8_t type, const std::string &data)
		{
			std::string buffer;
			encode_uint64(buffer, gen_version());
			encode_uint8(buffer, type);
			encode_string(buffer, data);
			wal_.write(buffer.data(), buffer.size());
		}

		void node::load_wal(std::ifstream &file)
		{
			do 
			{
				auto version = decode_uint64(file);
				if (file.eof())
					return;
				auto type = decode_uint8(file);
				auto data = decode_string(file);
				if(version <= version_)
					continue;

				if (type == wal_type::e_regist_engine)
				{
					engine_info info;
					info.ParseFromString(data);
					engines_[info.engine_name()] = info;
				}
				else if (type == wal_type::e_regist_engine)
				{
				}
			} while (true);
		}

	}

}

