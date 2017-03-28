#pragma once
namespace romi
{
	//

	inline std::string serialize_to_string(sys::engine_offline &)
	{
		return{};
	}
	inline std::string serialize_to_string(sys::actor_close &)
	{
		return{};
	}
	inline std::string serialize_to_string(sys::actor_init &)
	{
		return{};
	}

	inline std::string serialize_to_string(sys::not_find_remote_engine &)
	{
		return{};
	}
	inline std::string serialize_to_string(sys::timer_expire&)
	{
		return{};
	}
	inline std::string serialize_to_string(sys::net_connect_result&)
	{
		return{};
	}

	inline std::string serialize_to_string(sys::net_send_failed&)
	{
		return{};
	}

	inline std::string serialize_to_string(std::string &)
	{
		return{};
	}

	inline std::string serialize_to_string(sys::add_watcher&)
	{
		return{};
	}

	inline std::string serialize_to_string(sys::del_watcher&)
	{
		return{};
	}


	

	//
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
			return value_.get();
		else
			return nullptr;
	}

	template<typename T>
	std::string romi::message<T>::to_data()
	{
		std::string value_buffer = serialize_to_string(*value_);
		std::string buffer;
		buffer.resize(get_sizeof(from_) +
			get_sizeof(to_) +
			get_sizeof(type_) +
			get_sizeof(value_buffer));

		uint8_t *ptr = (uint8_t *)buffer.data();
		encode_uint64(ptr, from_.actor_id_);
		encode_uint64(ptr, from_.engine_id_);
		encode_uint64(ptr, to_.actor_id_);
		encode_uint64(ptr, to_.engine_id_);
		encode_string(ptr, type_);
		encode_string(ptr, value_buffer);
		return buffer;
	}

	template<typename T>
	message_base::ptr message<T>::make_shared(const void* buffer, std::size_t len)
	{
		uint8_t *ptr = (uint8_t*)buffer;
		auto msg = std::make_shared<message<T>>();
		msg->from_.actor_id_ = decode_uint64(ptr);
		msg->from_.engine_id_ = decode_uint64(ptr);
		msg->to_.actor_id_ = decode_uint64(ptr);
		msg->to_.engine_id_ = decode_uint64(ptr);
		msg->type_ = decode_string(ptr);

		auto value_buffer = decode_string(ptr);
		msg->value_.reset(build_message<T>(msg->type_, value_buffer.data(), value_buffer.size()));
		return msg;
	}

	template<typename T>
	inline  std::shared_ptr<romi::message_base>
		make_message(const addr &from, const addr &to, T &&val)
	{
		return std::make_shared<message<std::decay_t<T>>>(from, to, std::forward<T>(val));
	}

	template<typename T>
	inline romi::message<T>::message(const addr &from, const addr &to, T &&value)
		: value_(new T(std::move(value)))//copy
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
		message_build::instance().regist([](const void *buffer, std::size_t len) {
			return message<Message>::make_shared(buffer, len);
		});
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