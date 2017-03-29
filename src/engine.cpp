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

	void engine::init()
	{
		net_.bind_handle_msg([this](void*, std::size_t) {

		});
		net_.bind_send_msg_to_actor([this](message_base::ptr msg) {
			send(std::move(msg));
		});
	}

	uint64_t engine::gen_actor_id()
	{
		return next_actor_id++;
	}

	void engine::send(message_base::ptr &&msg)
	{
		if (msg->from_.engine_id() == engine_id_)
		{
			if (const auto _actor = find_actor(msg->to_))
			{
				if (!_actor->receive_msg(std::move(msg)))
				{
					dispatcher_pool_.dispatch(_actor);
				}
			}
			
		}
		else
		{
			send_to_net(std::move(msg));
		}
	}


	void engine::set_config(config cfg)
	{
		config_ = cfg;
	}

	void engine::start()
	{
		assert(!is_start_);
		is_start_ = true;
		timer_.start();
		dispatcher_pool_.start(config_.dispatcher_pool_size);
		net_.bind(config_.zeromq_bind_port_);
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
		_actor->addr_.set_actor_id(gen_actor_id());
		_actor->addr_.set_engine_id(engine_id_);

		_actor->set_timer_ = [this](addr _addr, std::size_t _delay, timer_id id) {
			return timer_.set_timer(_delay, [=] {
				sys::timer_expire expire;
				expire.set_timer_id(id);
				send(make_message(_addr, _addr, expire));
				return true;
			});
		};

		_actor->cancel_timer_ = [this](timer_id id) { 
			timer_.cancel_timer(id); 
		};

		_actor->add_watcher_ = [this](addr from, addr _actor) {
			if (_actor.engine_id() == engine_id_)
			{
				sys::add_watcher _add_watcher;
				auto tmp = _add_watcher.mutable_addr();
				tmp->set_actor_id(_actor.actor_id());
				tmp->set_engine_id(_actor.engine_id());
				return send(make_message(from, _actor, _add_watcher));
			}
			add_remote_watcher(from, _actor);
		};
		
		_actor->cancel_watch_ = [this](addr from, addr _actor) {
			if (_actor.engine_id() == engine_id_)
			{
				sys::del_watcher _del_watcher;
				auto addr_ptr= _del_watcher.mutable_addr();
				*addr_ptr = _actor;
				return send(make_message(from, _actor, _del_watcher));
			}
			del_remote_watcher(from, _actor);
		};
		
		_actor->close_callback_ = [this](addr _addr) {
			del_actor(_addr); 
		};
		
		send(make_message(_actor->addr_, _actor->addr_, sys::actor_init()));
		add_actor(_actor);
	}


	romi::actor::ptr engine::find_actor(addr &_addr)
	{
		std::lock_guard<std::mutex> lg(actors_.lock_);
		return actors_.actors_[_addr];
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

	void engine::send_to_net(message_base::ptr &&message_)
	{

	}

	void engine::add_remote_watcher(addr from, addr _actor)
	{
		add_engine_watcher(from, _actor);
		sys::add_watcher _add_watcher;
		*_add_watcher.mutable_addr() = _actor;
		send_to_net(make_message(from, _actor, _add_watcher));
	}


	void engine::del_remote_watcher(addr from, addr _actor)
	{
		del_engine_watcher(from, _actor);
		sys::del_watcher _del_watcher;
		auto _addr = _del_watcher.mutable_addr();
		*_addr = _actor;
		send_to_net(make_message(from, _actor, _del_watcher));
	}

	void engine::add_engine_watcher(addr from, addr _actor)
	{
		std::lock_guard<std::mutex> lock_guard_(engine_watcher_.locker_);
		engine_watcher_.watchers_[_actor.engine_id()].insert(from);
	}


	void engine::del_engine_watcher(addr from, addr _actor)
	{
		std::lock_guard<std::mutex> lock_guard_(engine_watcher_.locker_);
		engine_watcher_.watchers_[_actor.engine_id()].erase(from);
	}

}