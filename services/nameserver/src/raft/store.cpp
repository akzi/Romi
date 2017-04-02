#include "store.h"

namespace romi
{
namespace raft
{

	/*
	data ColumnFamily name begin with #
	#raft_id

	data ColumnFamily name begin with $
	$raft_id:index

	index range : [
	*/

	store::~store()
	{
		std::lock_guard<std::mutex> locker(data_cf_mutex_);
		std::lock_guard<std::mutex> locker2(log_cf_mutex_);
		if (!db_)
			return;

		for (auto &map: data_cf_handles_)
		{
			db_->DestroyColumnFamilyHandle(map.second);
		}

		for (auto &map: log_cf_handles_)
		{
			for (auto map_itr: map.second)
			{
				db_->DestroyColumnFamilyHandle(map_itr.second);
			}
		}
		data_cf_handles_.clear();
		log_cf_handles_.clear();
		delete db_;
		db_ = nullptr;
	}

	store & store::get_instance(const std::string &db_path /*= {}*/)
	{
		static store instance(db_path);
		return instance;
	}

	bool store::put_meta(const std::string &name,
		const std::string &key, const std::string &value)
	{
		return db_->Put({}, name + "_$_" + key, value).ok();
	}

	bool store::get_meta(const std::string &name, 
		const std::string &key, std::string &value)
	{
		return db_->Get({}, name + "_$_" + key, &value).ok();
	}

	bool store::put_data(const std::string &name, 
		const std::list<std::pair<std::string, std::string>> &values)
	{
		auto cf_handle = get_data_cf("#" + name);
		if (!cf_handle)
			return false;

		rocksdb::WriteBatch batch;
		for (auto &itr : values)
		{
			batch.Put(cf_handle, itr.first, itr.second);
		}

		return db_->Write({}, &batch).ok();
	}

	bool store::put_data(const std::string &name, 
		const std::string &key, const std::string &value)
	{
		auto cf_handle = get_data_cf("#" + name);
		if (!cf_handle)
			return false;
		return db_->Put({}, cf_handle, key, value).ok();
	}

	bool store::get_data(const std::string &name, 
		const std::string &key, std::string &value)
	{
		auto cf_handle = get_data_cf("#" + name);
		if (!cf_handle)
			return false;
		return db_->Get({}, cf_handle, key, &value).ok();
	}

	bool store::append_log(const std::string &name,
		const std::list<std::pair<uint64_t, std::string>> &entries)
	{
		uint64_t begin = 0;
		uint64_t index = entries.front().first;
		auto cf_handle = find_log_cf("$" + name, &begin, index);
		if (!cf_handle || index - begin > max_log_entries_size_)
		{
			cf_handle = new_log_cf("$" + name, index);
		}
		if (!cf_handle)
			return false;

		rocksdb::WriteBatch batch;
		for (auto &itr : entries)
		{
			batch.Put(cf_handle, std::to_string(itr.first), itr.second);
		}
		return db_->Write({}, &batch).ok();
	}

	bool store::append_log(const std::string &name, 
		const std::pair<uint64_t, std::string> &entry)
	{
		uint64_t begin = 0;
		uint64_t index = entry.first;

		auto cf_handle = find_log_cf("$" + name, &begin, index);
		if (!cf_handle || index - begin > max_log_entries_size_)
		{
			cf_handle = new_log_cf("$" + name, index);
		}
		if (!cf_handle)
			return false;
		auto value = entry.second;
		return db_->Put({}, cf_handle, std::to_string(index), value).ok();
	}

	bool store::get_log(const std::string &name, 
		uint16_t index, std::string &entry)
	{
		auto cf_handle = find_log_cf("$" + name, nullptr, index);
		if (!cf_handle)
			return false;

		return db_->Get(rocksdb::ReadOptions(true, true),
			std::to_string(index), &entry).ok();
	}

	bool store::get_first_log(const std::string &name, 
		uint64_t & index, std::string &entry)
	{
		auto cf_map = get_log_cf_map(name);
		if (cf_map.empty())
			return false;

		auto iter = db_->NewIterator(rocksdb::ReadOptions(true, true), 
			cf_map.begin()->second);

		iter->SeekToFirst();
		auto key = iter->key();
		auto slice = iter->value();
		index = std::strtoull(key.data(), 0, 10);
		entry.append(slice.data(), slice.size());
		return true;
	}

	bool store::get_last_log(const std::string &name,
		uint64_t & index, std::string &entry)
	{
		auto cf_map = get_log_cf_map(name);
		if (cf_map.empty())
			return false;

		auto iter = db_->NewIterator(rocksdb::ReadOptions(true, true),
			cf_map.rbegin()->second);

		iter->SeekToLast();
		auto key = iter->key();
		auto slice = iter->value();
		index = std::strtoull(key.data(), 0, 10);
		entry.append(slice.data(), slice.size());
		return true;
	}

	bool store::get_logs(
		const std::string &name, 
		uint64_t index,
		uint32_t max_bytes, 
		std::list<std::pair<uint64_t, std::string>> &entries,
		uint32_t &bytes_)
	{
		auto cf_handle = find_log_cf("$" + name, nullptr, index);
		if (!cf_handle)
			return false;

		auto iter = db_->NewIterator(rocksdb::ReadOptions(true, true), cf_handle);
		for (iter->SeekToFirst(); iter->Valid() ; iter->Next())
		{
			auto key = iter->key();
			auto value = iter->value();
			auto num = std::strtoull(key.data(), 0, 10);
			entries.emplace_back(num, value);
			bytes_ += value.size();

			//At least one
			if (max_bytes < bytes_)
				return true;
		}
		if (bytes_ < max_bytes)
			get_logs(name, entries.back().first + 1, max_bytes, entries, bytes_);

		return true;
	}

	bool store::get_logs(
		const std::string &name,
		uint64_t index, 
		uint32_t count, 
		std::list<std::pair<uint64_t, std::string>> &entries)
	{
		auto cf_handle = find_log_cf("$" + name, nullptr, index);
		if (!cf_handle)
			return false;

		auto iter = db_->NewIterator(rocksdb::ReadOptions(true, true), cf_handle);
		for (iter->SeekToFirst(); iter->Valid(); iter->Next())
		{
			auto key = iter->key();
			auto value = iter->value();
			auto num = std::strtoull(key.data(), 0, 10);
			entries.emplace_back(num, value);
			if (count < entries.size())
				return true;
		}
		if (entries.size() < count)
			get_logs(name, entries.back().first + 1, count, entries);

		return true;
	}

	void store::drop_excess_log_cf(const std::string &name, int max_size)
	{
		auto cf_map = get_log_cf_map(name);
		if (cf_map.size() < max_size)
			return;
		do
		{
			auto cf_handle = cf_map.begin()->second;
			auto state = db_->DropColumnFamily(cf_handle);
			assert(state.ok());
			state = db_->DestroyColumnFamilyHandle(cf_handle);
			assert(state.ok());
			cf_map.erase(cf_map.begin());

		} while (cf_map.size() > max_size);
	}

	rocksdb::ColumnFamilyHandle * 
		store::new_log_cf(const std::string &cf_name, uint64_t index)
	{
		rocksdb::ColumnFamilyHandle * cf_handle = nullptr;
		auto name = cf_name + ":" + std::to_string(index);//$cf_name:index

		auto state = db_->CreateColumnFamily({}, name, &cf_handle);
		if (state.ok())
		{
			std::lock_guard<std::mutex> locker(log_cf_mutex_);
			log_cf_handles_[cf_name][index] = cf_handle;
		}
		return cf_handle;
	}

	rocksdb::ColumnFamilyHandle*
		store::find_log_cf(const std::string &cf_name, 
			uint64_t *begin, uint64_t index)

	{
		rocksdb::ColumnFamilyHandle * cf_handle = nullptr;

		auto cf_map = get_log_cf_map(cf_name);
		if (cf_map.empty())
		{
			auto state = db_->CreateColumnFamily({}, cf_name, &cf_handle);
			if (!state.ok())
				return cf_handle;
			cf_map[index] = cf_handle;
			return cf_handle;
		}

		for (auto &itr : cf_map)
		{
			if (itr.first <= index)
			{
				if (begin)
					*begin = itr.first;
				cf_handle = itr.second;
			}
		}
		return cf_handle;
	}

	std::map<uint64_t, rocksdb::ColumnFamilyHandle *> &
		store::get_log_cf_map(const std::string &cf_name)
	{
		std::lock_guard<std::mutex> locker(log_cf_mutex_);
		return log_cf_handles_[cf_name];
	}

	rocksdb::ColumnFamilyHandle * store::find_data_cf(const std::string &cf_name)
	{
		std::lock_guard<std::mutex> locker(data_cf_mutex_);
		return data_cf_handles_[cf_name];
	}

	rocksdb::ColumnFamilyHandle * store::get_data_cf(const std::string &cf_name)
	{
		auto cf_handle = find_data_cf(cf_name);
		if (cf_handle)
			return cf_handle;

		auto state = db_->CreateColumnFamily({}, cf_name, &cf_handle);
		if (state.ok())
		{
			std::lock_guard<std::mutex> locker(data_cf_mutex_);
			data_cf_handles_[cf_name] = cf_handle;
		}
		return cf_handle;
	}

	store::store(const std::string &db_path)
	{
		db_options_.base_background_compactions = 2;
		db_options_.max_background_compactions = 6;
		db_options_.max_background_flushes = 6;
		db_options_.create_if_missing = true;

		std::vector<rocksdb::ColumnFamilyHandle*> handles;
		std::vector<rocksdb::ColumnFamilyDescriptor> CFDs;
		std::vector<std::string> CF_names;

		std::lock_guard<std::mutex> locker(data_cf_mutex_);
		std::lock_guard<std::mutex> locker2(log_cf_mutex_);

		auto status = rocksdb::DB::ListColumnFamilies(
			db_options_, db_path, &CF_names);

		assert(status.ok());

		for (auto itr : CF_names)
		{
			rocksdb::ColumnFamilyDescriptor CFD;
			CFD.name = itr;
			CFDs.push_back(CFD);
		}
		status = rocksdb::DB::Open(db_options_, 
			db_path, CFDs, &handles, &db_);

		assert(status.ok());
		if (handles.empty())
			return;

		for (auto itr : handles)
		{
			auto name = itr->GetName();
			if (name == "default")
			{
				assert(default_cf_handle_);
				default_cf_handle_ = itr;
				continue;
			}
			else if(name[0] == '#')
			{
				auto cf_name = name.substr(1, name.size()-1);
				data_cf_handles_[cf_name] = itr;
			}
			else if (name[0] == '$')
			{
				auto pos = name.find(':');
				auto cf_name = name.substr(1, pos);
				auto subfix = name.substr(pos + 1);
				auto index = std::strtoull(subfix.c_str(), 0, 10);
				log_cf_handles_[cf_name][index] = itr;
			}
		}
	}
}
}

