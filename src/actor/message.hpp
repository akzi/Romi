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
		template<typename T> T* get()const
		{
			return static_cast<T*>(get_impl(typeid(std::decay_t<T>)));
		}

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
		virtual void* get_impl(const std::type_info&info) const
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
		virtual void* get_impl(const std::type_info& info)const ;
		std::string serialize_as_string();
	};

	template<typename T>
	message_base::ptr make_message(const addr &, const addr &, const T &);
}


