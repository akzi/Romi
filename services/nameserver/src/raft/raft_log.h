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
	};
}
}