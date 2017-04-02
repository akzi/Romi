#include <romi.hpp>
#include <raft.pb.h>
#include <raft_log.h>
#include <store.h>

namespace romi
{
namespace raft
{
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
}
}

