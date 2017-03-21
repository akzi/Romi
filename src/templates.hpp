#pragma once
namespace romi
{
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
		auto func = [handle](std::shared_ptr<message_base> msg) {
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