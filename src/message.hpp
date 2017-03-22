#pragma once
namespace romi
{

#define ROMI_DEFINE_MSG(Sym, Message) \
	template<>  struct message_traits<Message> \
	{  enum { value = 1 }; static constexpr char *_$_type = ""#Sym#Message;}

#define ROMI_DEFINE_ACTOR_MSG(Message) ROMI_DEFINE_MSG([A], Message);
#define ROMI_DEFINE_EVENT_MSG(Message) ROMI_DEFINE_MSG([E], Message);
#define ROMI_DEFINE_SYS_MSG(Message) ROMI_DEFINE_MSG([S], Message);

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
		template<typename T> T* get();
	private:
		virtual void* get_impl(const std::type_info&info) { return nullptr; }
	};


	template<typename T>
	class message : public message_base
	{
		T value_;
	public:
		message(const addr &from, const addr &to, T value);
		virtual void* get_impl(const std::type_info& info);
	};

	//sys
	namespace sys
	{
		struct actor_init { };
		struct engine_offline { };
		struct timer_expire { timer_id id_; };
	}

	ROMI_DEFINE_SYS_MSG(sys::actor_init);
	ROMI_DEFINE_SYS_MSG(sys::timer_expire);

	//event
	namespace event
	{
		struct add_actor_watcher { addr actor_; };
		struct del_actor_watcher { addr actor_; };
		struct add_engine_watcher { engine_id engid_id_; };
		struct del_engine_watcher { engine_id engid_id_; };
	}

	ROMI_DEFINE_EVENT_MSG(event::add_actor_watcher);
	ROMI_DEFINE_EVENT_MSG(event::del_actor_watcher);
	ROMI_DEFINE_EVENT_MSG(event::add_engine_watcher);
	ROMI_DEFINE_EVENT_MSG(event::del_engine_watcher);

	template<typename T>
	std::shared_ptr<message_base> 
		make_message(const addr &from, const addr &to, T &&val);
}