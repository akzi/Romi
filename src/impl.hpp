#pragma once
namespace romi
{
	//addr
	inline bool operator == (const addr &left, const addr &right)
	{
		return left.engine_id_ == right.engine_id_ &&
			left.actor_id_ == right.actor_id_;
	}

	//message_base
	template<typename T>
	inline T* romi::message_base::get()
	{
		return static_cast<T*>(get_impl(typeid(std::decay_t<T>)));
	}


	//message
	template<typename T>
	inline void* romi::message<T>::get_impl(const std::type_info& info)
	{
		if (typeid(T) == info)
			return &value_;
		else
			return nullptr;
	}

	template<typename T>
	std::string romi::message<T>::to_data()
	{

	}

	template<typename T>
	inline  std::shared_ptr<romi::message_base>
		make_message(const addr &from, const addr &to, T &&val)
	{
		return std::make_shared<message<std::decay_t<T>>>(from, to, std::forward<T>(val));
	}

	template<typename T>
	inline romi::message<T>::message(const addr &from, const addr &to, T value)
		: value_(std::move(value))
	{
		from_ = from;
		to_ = to;
		type_ = get_message_type<T>();
	}

	//actor
	template<typename T>
	inline void actor::send(const addr &to, T &&obj)
	{
		send_msg_(make_message(addr_, to, std::forward<T>(obj)));
	}

	template<typename Handle>
	inline void actor::receivce(Handle handle)
	{
		receivce_help(to_function(std::forward<Handle>(handle)));
	}

	template<typename Message>
	inline void actor::receivce_help(std::function<void(const addr&, const Message &)> handle)
	{
		auto func = [handle](message_base::ptr msg) {
			Message *message = msg->get<Message>();
			assert(message);
			handle(msg->from_, *message);
		};
		msg_handles_[get_message_type<Message>()] = func;
	}


	//engine
	template<typename Actor, typename ...Args>
	inline std::enable_if_t<std::is_base_of<actor, Actor>::value, addr>
		engine::spawn(Args &&...args)
	{
		actor::ptr _actor = std::make_shared<Actor>(std::forward<Args>(args)...);
		init_actor(_actor);
		return _actor->addr_;
	}

	template<typename T>
	inline std::enable_if_t<message_traits<T>::value>
		engine::send(const addr &from, const addr &to, T &&msg)
	{
		send(make_message(from, to, std::forward<T>(msg)));
	}
}