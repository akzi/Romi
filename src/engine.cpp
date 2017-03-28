#include "romi.hpp"
namespace romi
{
	using lock_guad = std::lock_guard<std::mutex>;
	engine::engine()
	{

	}
	engine::~engine()
	{
		if (is_start_)
			stop();
	}

	actor_id engine::gen_actor_id()
	{
		return next_actor_id++;
	}

	void engine::send(message_base::ptr &&msg)
	{
		if (const auto _actor = msg->to_.actor_.lock())
		{
			assert(msg->from_.engine_id_ == engine_id_);
			if (!_actor->receive_msg(std::move(msg)))
			{
				dispatcher_pool_.dispatch(_actor);
			}
		}
		else
		{
			send_to_net(std::move(msg));
		}
	}


	void engine::bind(int port)
	{
		port_ = port;
	}

	void engine::start()
	{
		assert(!is_start_);
		is_start_ = true;
		timer_.start();
		dispatcher_pool_.start();
		net_.bind(port_);
		net_.start();
	}

	void engine::stop()
	{
		assert(is_start_);
		is_start_ = false;
		timer_.stop();
		dispatcher_pool_.stop();
	}

	void engine::init_actor(actor::ptr &_actor)
	{
		_actor->send_msg_ = [this](auto &&msg) {
			send(std::move(msg));
		};
		_actor->addr_.actor_ = _actor;
		_actor->addr_.actor_id_ = gen_actor_id();
		_actor->addr_.engine_id_ = engine_id_;

		_actor->set_timer_ = [this](addr _addr, std::size_t _delay, timer_id id) {
			return timer_.set_timer(_delay, [=] {
				send(make_message(_addr, _addr, sys::timer_expire{ id }));
				return true;
			});
		};

		_actor->cancel_timer_ = [this](timer_id id) { 
			timer_.cancel_timer(id); 
		};

		_actor->add_watcher_ = [this](addr from, addr _actor) {
			if (_actor.engine_id_ == engine_id_)
				return send(make_message(from, _actor, sys::add_watcher{ from }));
			add_remote_watcher(from, _actor);
		};
		
		_actor->cancel_watch_ = [this](addr from, addr _actor) {
			if (_actor.engine_id_ == engine_id_)
				return send(make_message(from, _actor, sys::del_watcher{ from }));
			del_remote_watcher(from, _actor);
		};
		
		_actor->close_callback_ = [this](addr _addr) {
			del_actor(_addr); 
		};
		
		send(make_message(_actor->addr_, _actor->addr_, sys::actor_init()));
		add_actor(_actor);
	}

	void engine::add_actor(actor::ptr &_actor)
	{
		std::lock_guard<std::mutex> lg(actors_.lock_);
		actors_.actors_.emplace(_actor->addr_, _actor);
	}


	void engine::del_actor(addr &_addr)
	{
		std::lock_guard<std::mutex> lg(actors_.lock_);
		actors_.actors_.erase(_addr);
	}

	void engine::send_to_net(message_base::ptr &&msg)
	{
		if (const auto socket = find_socket(msg->to_.engine_id_))
		{
			sys::net_send *send_msg = new sys::net_send;
		}
	}

	void engine::add_remote_watcher(addr from, addr _actor)
	{
		add_engine_watcher(from, _actor);
		send_to_net(make_message(from, _actor, sys::add_watcher{ from }));
	}


	void engine::del_remote_watcher(addr from, addr _actor)
	{
		del_engine_watcher(from, _actor);
		send_to_net(make_message(from, _actor, sys::del_watcher{ from }));
	}

	void engine::add_engine_watcher(addr from, addr _actor)
	{
		std::lock_guard<std::mutex> lock_guard_(engine_watcher_.locker_);
		engine_watcher_.watchers_[_actor.engine_id_].insert(from);
	}


	void engine::del_engine_watcher(addr from, addr _actor)
	{
		std::lock_guard<std::mutex> lock_guard_(engine_watcher_.locker_);
		engine_watcher_.watchers_[_actor.engine_id_].erase(from);
	}

}