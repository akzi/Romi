#pragma once
namespace romi
{
	//

	//addr
	inline bool operator == (const addr &left, const addr &right)
	{
		return left.engine_id() == right.engine_id() &&
			left.actor_id() == right.actor_id();
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
	std::string romi::message<T>::serialize_as_string()
	{
		std::string buffer;
		std::string from = from_.SerializeAsString();
		std::string to = to_.SerializeAsString();
		std::string value = value_->SerializeAsString();
		buffer.resize(get_sizeof(from) + 
			get_sizeof(to) + 
			get_sizeof(type_) + 
			get_sizeof(value));

		uint8_t *ptr = (uint8_t*)buffer.data();
		encode_string(ptr, from);
		encode_string(ptr, to);
		encode_string(ptr, type_);
		encode_string(ptr, value);

		return buffer;
	}

	template<typename T>
	message_base::ptr message<T>::parse_from_array(const void* data, std::size_t len)
	{
		uint8_t *ptr = (uint8_t*)data;
		auto msg = std::make_shared<message<T>>();

		auto value_buffer = decode_string(ptr);
		auto obj = build_message<T>(msg->type_, value_buffer.data(), value_buffer.size());
		return msg;
	}

	template<typename T>
	inline  std::shared_ptr<romi::message_base>
		make_message(const addr &from, const addr &to, const T &val)
	{
		return std::make_shared<message<std::decay_t<T>>>(from, to, val);
	}

	template<typename T>
	inline romi::message<T>::message(const addr &from, const addr &to, const T &value)
		: value_(new T(value))//copy
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
		message_build::instance().regist(get_message_type<Message>(), 
			[](const void *buffer, std::size_t len) {
			return message<Message>::parse_from_array(buffer, len);
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
	std::enable_if_t<std::is_base_of<::google::protobuf::Message, T>::value>
		engine::send(const addr &from, const addr &to, const T &msg)
	{
		send(make_message(from, to, msg));
	}
}