#pragma once
namespace romi
{
namespace raft
{
	class raft_log
	{
	public:
		raft_log()
		{

		}
		void init_store(std::string store_path);

		bool fill_log_entries(
			const std::string &raft_id_, 
			uint64_t index, 
			uint32_t max_bytes, 
			raft::replicate_log_entries_request &req);

		raft::log_entry get_log_entry(
			const std::string raft_id, 
			uint64_t index);

		bool get_last_log_entry(
			const std::string name,
			raft::log_entry &entry);

		void write_raft_log(
			const std::string &raft_id, 
			const std::list<std::pair<uint64_t, std::string>> & entries);

		void write_raft_log(
			const std::string &raft_id, 
			std::pair<uint64_t, std::string> & entry);

		bool get_log_entrys(
			std::string raft_id_, 
			uint64_t index, 
			uint32_t count, 
			std::list<std::pair<uint64_t, std::string>> &entries);

		void truncate_suffix(const std::string &raft_id,
			uint64_t index);
	};
}
}