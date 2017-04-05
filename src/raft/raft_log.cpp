#include <romi.hpp>
#include <romi.raft.pb.h>
#include <node.h>
#include <store.h>

namespace romi
{
namespace raft
{

	void raft_log::init_store(std::string store_path)
	{
		store::get_instance(store_path);
	}

	bool raft_log::fill_log_entries(
		const std::string &raft_id_,
		uint64_t index, 
		uint32_t max_bytes, 
		raft::replicate_log_entries_request &req)
	{
		uint32_t bytes = 0;
		if (!max_bytes)
			max_bytes = 1024 * 1024;//1MB

		std::list<std::pair<uint64_t, std::string>> entries;
		if (!store::get_instance().get_logs(raft_id_, index,
			max_bytes, entries, bytes) || entries.empty())
			return false;

		for (auto &itr: entries)
		{
			auto log_entry = req.add_entries();
			log_entry->ParseFromString(itr.second);
		}
		return true;
	}

	raft::log_entry raft_log::get_log_entry(const std::string raft_id, uint64_t index)
	{
		std::string buffer;
		raft::log_entry entry;
		
		if (!store::get_instance().get_log(raft_id, index, buffer))
			throw std::runtime_error("store.get_log failed");
		
		entry.ParseFromString(buffer);
		return entry;
	}

	bool raft_log::get_last_log_entry(
		const std::string name,
		raft::log_entry &entry)
	{
		std::string buffer;
		if (store::get_instance().get_last_log(name, buffer))
		{
			entry.ParseFromString(buffer);
			return true;
		}
		return false;
	}

	void raft_log::write_raft_log(const std::string &raft_id,
		const std::list<std::pair<uint64_t, std::string>> & entries)
	{
		if (entries.size())
		{
			if (!store::get_instance().append_log(raft_id, entries))
			{
				throw std::runtime_error("store append_log failed");
			}
		}		
	}

	void raft_log::write_raft_log(const std::string &raft_id, 
		std::pair<uint64_t, std::string> & entry)
	{
		if (!store::get_instance().append_log(raft_id, entry))
			throw std::runtime_error("store append_log failed");
	}

	bool raft_log::get_log_entrys(
		std::string raft_id_, 
		uint64_t index, 
		uint32_t count, 
		std::list<std::pair<uint64_t, std::string>> &entries)
	{
		return store::get_instance().get_logs(raft_id_, index, count, entries);
	}

	void raft_log::truncate_suffix(const std::string &raft_id, uint64_t index)
	{
		store::get_instance().truncate_suffix(raft_id, index);
	}

}
}

