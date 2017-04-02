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
		bool fill_log_entries(
			const std::string &raft_id_, 
			uint64_t index, 
			uint32_t max_bytes, 
			raft::replicate_log_entries_request &req);

		raft::log_entry get_log_entry(
			const std::string raft_id, 
			uint64_t index);

		void write_raft_log(
			const std::string &raft_id, 
			const std::list<std::pair<uint64_t, std::string>> & entries);

		bool get_log_entrys(
			std::string raft_id_, 
			uint64_t index, 
			uint32_t count, 
			std::list<std::pair<uint64_t, std::string>> &entries);
	};
}
}