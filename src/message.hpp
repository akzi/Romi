#pragma once
namespace romi
{

#define ROMI_DEFINE_MSG(Sym, Message) \
	template<>  struct message_traits<Message> \
	{  enum { value = 1 }; static constexpr char *_$_type = ""#Sym#Message;}

#define ROMI_DEFINE_ACTOR_MSG(Message) ROMI_DEFINE_MSG(., Message);
#define ROMI_DEFINE_SYS_MSG(Message) ROMI_DEFINE_MSG(?, Message);

	template<typename T>
	struct message_traits
	{
		enum { value = 0 };
	};

	ROMI_DEFINE_ACTOR_MSG(std::string);

	template<typename T>
	inline constexpr std::enable_if_t<message_traits<T>::value, const char *>
		get_message_type()
	{
		return message_traits<std::decay_t<T>>::_$_type;
	}

	template<typename T>
	inline constexpr std::enable_if_t<!message_traits<T>::value, const char *>
		get_message_type()
	{
		return typeid(T).name();
	}


	struct message_base
	{
		using ptr = std::shared_ptr<message_base>;
		addr from_;
		addr to_;
		std::string type_;
		virtual ~message_base() {}
		template<typename T> T* get()
		{
			return static_cast<T*>(get_impl(typeid(std::decay_t<T>)));
		}
	private:
		virtual void* get_impl(const std::type_info& info) { return nullptr; }
	};


	template<typename T>
	class message : public message_base
	{
		T value_;
	public:
		message(const addr &from, const addr &to, T value)
			: value_(std::move(value))
		{
			from_ = from;
			to_ = to;
			type_ = get_message_type<T>();
		}

		virtual void* get_impl(const std::type_info& info)
		{
			if (typeid(T) == info)
				return &value_;
			else
				return nullptr;
		}
	};
	namespace sys
	{
		struct actor_init { };
		struct engine_offline { };
	}

	ROMI_DEFINE_SYS_MSG(sys::actor_init);

	template<typename T>
	inline  std::shared_ptr<message_base> make_message(const addr &from, const addr &to, T &&val)
	{
		return std::make_shared<message<T>>(from, to, std::forward<T>(val));
	}
}