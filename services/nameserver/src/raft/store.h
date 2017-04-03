#pragma once
#include <rocksdb/db.h>
#include <mutex>
#include "raft.pb.h"

namespace romi
{
namespace raft
{

	class store
	{
	public:

		~store();

		static store &get_instance(const std::string &db_path = {});


		bool put_meta(
			const std::string &name,
			const std::string &key,
			const std::string &value);

		bool get_meta(
			const std::string &name,
			const std::string &key,
			std::string &value);

		bool put_data(
			const std::string &name,
			const std::string &key,
			const std::string &value);

		bool put_data(
			const std::string &name,
			const std::list<std::pair<std::string, std::string>> &values);

		bool get_data(
			const std::string &name,
			const std::string &key,
			std::string &value);


		bool append_log(
			const std::string &name,
			const std::pair<uint64_t, std::string> &entry);

		bool append_log(
			const std::string &name,
			const std::list<std::pair<uint64_t, std::string>> &entries);

		bool get_log(
			const std::string &name,
			uint64_t  index, std::string &entry);

		bool get_first_log(
			const std::string &name,
			uint64_t & index,
			std::string &entry);

		bool get_last_log(const std::string &name,
			uint64_t & index, std::string &entry);


		bool get_logs(
			const std::string &name,
			uint64_t index,
			uint32_t max_bytes,
			std::list<std::pair<uint64_t, std::string>> &entries,
			uint32_t &bytes_);

		bool get_logs(
			const std::string &name,
			uint64_t index,
			uint32_t count,
			std::list<std::pair<uint64_t, std::string>> &entries);

		void truncate_suffix(const std::string & name, 
			uint64_t index);

	private:
		store(const std::string &db_path);

		void drop_excess_log_cf(const std::string &name, int max_size);

		rocksdb::ColumnFamilyHandle *new_log_cf(
			const std::string &cf_name, 
			uint64_t index);

		rocksdb::ColumnFamilyHandle *find_log_cf(
			const std::string &cf_name, 
			uint64_t *begin, 
			uint64_t index);

		std::map<uint64_t, rocksdb::ColumnFamilyHandle *> &
			get_log_cf_map(const std::string &cf_name);

		rocksdb::ColumnFamilyHandle *
			find_data_cf(const std::string &cf_name);

		rocksdb::ColumnFamilyHandle *
			get_data_cf(const std::string &cf_name);

		
		// log entries in one ColumnFamily 
		uint64_t max_log_entries_size_ = 1024 * 10;


		std::mutex log_cf_mutex_;

		std::map<std::string, std::map<uint64_t, 
			rocksdb::ColumnFamilyHandle*>> log_cf_handles_;

		std::mutex data_cf_mutex_;
		std::map<std::string, 
			rocksdb::ColumnFamilyHandle*> data_cf_handles_;

		rocksdb::ColumnFamilyHandle* default_cf_handle_ = nullptr;
		rocksdb::Options db_options_;

		rocksdb::DB *db_ = nullptr;
	};
}
}