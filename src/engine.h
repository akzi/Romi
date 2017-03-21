#pragma once
namespace romi
{
	class engine
	{
	public:
		engine();
		~engine();

		template<typename Actor, typename ...Args>
		std::enable_if_t<std::is_base_of<actor, Actor>::value, addr>
			spawn(Args &&...args)
		{

		}

	private:
		void send(std::shared_ptr<message_base> &&msg);

		struct actors
		{
			std::mutex mutex_;
			std::map<addr, actor::ptr, addr::less> actors_;
		};

		engine_id id_;
		actors actors_;
	};
}