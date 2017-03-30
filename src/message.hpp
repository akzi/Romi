#pragma once
namespace romi
{

	struct message_base
	{
		using ptr = std::shared_ptr<message_base>;
		addr from_;
		addr to_;
		std::string type_;
		virtual ~message_base() 
		{
		
		}
		template<typename T> T* get();

		virtual std::string serialize_as_string() 
		{
			return std::string(); 
		}
		virtual std::string type() 
		{ 
			return type_;
		};
		addr from()
		{
			return from_;
		}
		addr to()
		{
			return to_;
		}
	private:
		virtual void* get_impl(const std::type_info&info) 
		{ 
			return nullptr; 
		}
	};


	template<typename T>
	class message : public message_base
	{
		std::shared_ptr<T> value_;		
	public:
		message() {}
		message(const addr &from, const addr &to, const T &value);
		static message_base::ptr parse_from_array(const void* buffer, std::size_t len);
	private:
		virtual void* get_impl(const std::type_info& info);
		std::string serialize_as_string();
	};

	template<typename T>
	message_base::ptr make_message(const addr &, const addr &, const T &);

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


