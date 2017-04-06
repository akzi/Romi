#pragma once
namespace romi
{
	class message_builder
	{
	public:
		using build_handle =
			std::function<message_base::ptr(const void*, std::size_t)>;
		static message_builder &instance()
		{
			static message_builder inst;
			return inst;
		}
		void regist(std::string message_type, const build_handle &handle)
		{
			std::lock_guard<std::mutex> locker(locker_);
			handles_[message_type] = handle;
		}
		build_handle get_message_build_handle(const std::string &message_type)
		{
			std::lock_guard<std::mutex> locker(locker_);
			return handles_[message_type];
		}
	private:
		std::mutex locker_;
		std::map<std::string, build_handle> handles_;
	};
}