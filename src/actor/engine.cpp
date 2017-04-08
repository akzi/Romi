#include "romi.hpp"
#include <future>


namespace romi
{
	constexpr uint64_t g_nameserver_actor_id = 1;
	constexpr uint64_t nameserver_engine_id = 1;
	engine::engine()
	{
	}
	engine::~engine()
	{
		if (is_start_)
			stop();
	}
	void engine::init_io_engine()
	{
		io_engine_.bind_handle_net_msg([this](void* data, std::size_t len) 
		{
			uint8_t *ptr = (uint8_t *)data;
			auto msg_type = decode_string(ptr);
			auto handle = message_builder::instance().
				get_message_build_handle(msg_type);

			if (!handle)
			{
				throw not_find_message_builder(msg_type);
			}
			auto msg = handle(data, len);
			if (!msg)
				throw build_message_error();
			handle_net_msg(msg);
		});
		io_engine_.bind_send_msg([this](message_base::ptr msg) {
			send(std::move(msg));
		});
	}

	void engine::init_message_builder()
	{
		REGIST_MESSAGE_BUILDER(sys::ping);
		REGIST_MESSAGE_BUILDER(sys::pong);
		REGIST_MESSAGE_BUILDER(sys::engine_offline);
	
		REGIST_MESSAGE_BUILDER(sys::net_connect);
		REGIST_MESSAGE_BUILDER(sys::net_connect_notify);
		REGIST_MESSAGE_BUILDER(sys::net_not_actor);
		REGIST_MESSAGE_BUILDER(sys::net_not_engine_id);
		REGIST_MESSAGE_BUILDER(sys::engine_offline);
		REGIST_MESSAGE_BUILDER(sys::timer_expire);
		REGIST_MESSAGE_BUILDER(sys::add_watcher);
		REGIST_MESSAGE_BUILDER(sys::del_watcher);
		REGIST_MESSAGE_BUILDER(sys::actor_close);

		REGIST_MESSAGE_BUILDER(nameserver::check_leader);
		REGIST_MESSAGE_BUILDER(nameserver::check_leader_result);
		REGIST_MESSAGE_BUILDER(nameserver::get_cluster_list_req);
		REGIST_MESSAGE_BUILDER(nameserver::get_cluster_list_resp);
		REGIST_MESSAGE_BUILDER(nameserver::regist_actor_req);
		REGIST_MESSAGE_BUILDER(nameserver::regist_actor_resp);
		REGIST_MESSAGE_BUILDER(nameserver::regist_actor_resp);
		REGIST_MESSAGE_BUILDER(nameserver::regist_engine_req);
		REGIST_MESSAGE_BUILDER(nameserver::regist_engine_resp);
		REGIST_MESSAGE_BUILDER(nameserver::get_engine_list_req);
		REGIST_MESSAGE_BUILDER(nameserver::get_engine_list_resp);
		REGIST_MESSAGE_BUILDER(nameserver::regist_actor_req);
		REGIST_MESSAGE_BUILDER(nameserver::regist_actor_resp);
		REGIST_MESSAGE_BUILDER(nameserver::find_actor_req);
		REGIST_MESSAGE_BUILDER(nameserver::find_actor_resp);
	}

	void engine::init_message_handle()
	{
		//
		regist_msg_handle(get_message_type<sys::ping>(),
			[this](const message_base::ptr &msg) 
		{
			if (msg->type() == get_message_type<sys::ping>())
			{
				return resp_pong(msg);
			}
		});

		regist_msg_handle(get_message_type<sys::pong>(),
			[this](const message_base::ptr &msg) 
		{
			if (const auto ptr = msg->get<sys::pong>())
			{
				handle_pong(msg);
			}
		});

		regist_msg_handle(get_message_type<sys::net_connect>(), 
			[this](const message_base::ptr &ptr) 
		{
			if (const auto msg = ptr->get<sys::net_connect>())
			{
				net::command cmd;
				cmd.net_connect_ = new sys::net_connect(*msg);
				cmd.type_ = net::command::e_net_connect;
				io_engine_.send_cmd(std::move(cmd));
			}
		});
	}

	uint64_t engine::gen_actor_id()
	{
		return next_actor_id++;
	}

	void engine::connect(uint64_t engine_id, const std::string &remote_addr)
	{
		net::command cmd;
		cmd.net_connect_ = new sys::net_connect;
		cmd.type_ = net::command::e_net_connect;
		cmd.net_connect_->mutable_remote_addr()->append(remote_addr);
		cmd.net_connect_->set_engine_id(engine_id);
		cmd.net_connect_->mutable_from()->set_actor_id(engine_id_);
		cmd.net_connect_->mutable_from()->set_engine_id(engine_actor_id_);

		io_engine_.send_cmd(std::move(cmd));
	}

	void engine::add_job(std::function<void()> &&handle)
	{
		assert(is_start_);
		threadpool_->add_job(std::move(handle));
	}


	void engine::add_job(const std::function<void()> &handle)
	{
		assert(is_start_);
		threadpool_->add_job(handle);
	}

	void engine::send(message_base::ptr &&msg)
	{
		if (msg->to_.engine_id() == engine_id_)
		{
			if (msg->to().actor_id() == engine_actor_id_)
			{
				if (auto handle = find_msg_handle(msg->type()))
				{
					handle(msg);
				}
			}
			else if (const auto _actor = find_actor(msg->to_))
			{
				if (_actor->receive_msg(std::move(msg)) == 1)
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
		assert(!is_start_);
		config_ = std::move(cfg);
		engine_id_ = config_.engine_id_;
		engine_addr_.set_actor_id(engine_actor_id_);
		engine_addr_.set_engine_id(engine_id_);
		default_nameserver_.set_engine_id(config_.nameserver_engine_id_);
		default_nameserver_.set_actor_id(config_.nameserver_actor_id_);
	}

	void engine::start()
	{
		assert(!is_start_);
		init_io_engine();
		init_message_builder();
		init_message_handle();
		timer_.start();
		threadpool_.reset(new threadpool(config_.threadpool_threads_));
		dispatcher_pool_.start(config_.dispatcher_pool_size);
		io_engine_.bind(config_.net_bind_addr_);
		io_engine_.start();

		timer_id_= timer_.set_timer(config_.net_heartbeart_timeout, 
			[this] {
			check_engine_watcher();
			return true;
		});
		is_start_ = true;
		if (!config_.regist_engine_ || config_.is_nameserver_)
			return;

		assert(config_.nameserver_addr_.size());

		init_nameserver_cluster();
		regist_engine();
	}

	void engine::stop()
	{
		assert(is_start_);
		is_start_ = false;
		io_engine_.stop();
		timer_.stop();
		dispatcher_pool_.stop();
		threadpool_->stop();
	}

	void engine::init_actor(actor::ptr &_actor)
	{
		
		_actor->engine_ = this;
		_actor->addr_.set_actor_id(gen_actor_id());
		_actor->addr_.set_engine_id(engine_id_);
		_actor->nameserver_addr_.set_engine_id(config_.nameserver_engine_id_);
		_actor->nameserver_addr_.set_actor_id(config_.nameserver_actor_id_);

		
		_actor->send_msg_ = [this](auto &&msg) {
			send(std::move(msg));
		}; 
		_actor->set_timer_ = [this](
			addr _addr, std::size_t _delay, uint64_t id) 
		{
			return timer_.set_timer(_delay, [=] {
				sys::timer_expire expire;
				expire.set_timer_id(id);
				send(make_message(_addr, _addr, expire));
				return true;
			});
		};

		_actor->cancel_timer_ = [this](uint64_t id) { 
			timer_.cancel_timer(id); 
		};

		_actor->add_watcher_ = [this](addr &from, addr& to) {
			if (to.engine_id() == engine_id_)
			{
				sys::add_watcher _add_watcher;
				auto tmp = _add_watcher.mutable_addr();
				tmp->set_actor_id(to.actor_id());
				tmp->set_engine_id(to.engine_id());
				return send(make_message(from, to, _add_watcher));
			}
			add_remote_watcher(from, to);
		};
		
		_actor->cancel_watch_ = [this](addr &from, addr &_actor) {
			if (_actor.engine_id() == engine_id_)
			{
				sys::del_watcher _del_watcher;
				auto addr_ptr= _del_watcher.mutable_addr();
				*addr_ptr = _actor;
				return send(make_message(from, _actor, _del_watcher));
			}
			del_remote_watcher(from, _actor);
		};
		
		_actor->close_callback_ = [this](addr &_addr) {
			del_actor(_addr); 
		};
		
		_actor->get_dispatcher_size_ = [this] {
			return dispatcher_pool_.size();
		};

		_actor->get_actor_size_ = [this] {
			std::lock_guard<std::mutex> locker(actors_.lock_);
			return actors_.actors_.size();
		};
		
		_actor->increase_dispather_ = [this](int count){
			dispatcher_pool_.increase(count);
		};

		add_actor(_actor);
		send(make_message(_actor->addr_, _actor->addr_, sys::actor_init()));
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

	void engine::handle_net_msg(message_base::ptr &msg)
	{
		if (msg->to_.engine_id() == engine_id_)
		{
			if (const auto _actor = find_actor(msg->to_))
			{
				if (_actor->receive_msg(std::move(msg)) == 1)
				{
					dispatcher_pool_.dispatch(_actor);
					return;
				}
			}
		}
		if (msg->to().actor_id() == engine_actor_id_)
		{
			if (auto handle = find_msg_handle(msg->type()))
			{
				handle(msg);
				return;
			}
		}
		std::cout << "can't " <<msg->type() << " msg handle" << std::endl;
	}

	void engine::send_to_net(message_base::ptr &message_)
	{
		net::command cmd;
		cmd.send_msg_ = new sys::send_msg;
		cmd.type_ = net::command::e_send_msg;
		cmd.send_msg_->message_ = message_;
		io_engine_.send_cmd(std::move(cmd));
	}

	void engine::add_remote_watcher(addr &from, addr& to)
	{
		add_engine_watcher(from, to);
		sys::add_watcher _add_watcher;
		*_add_watcher.mutable_addr() = from;
		send_to_net(make_message(from, to, _add_watcher));
	}


	void engine::del_remote_watcher(addr &from, addr &to)
	{
		del_engine_watcher(from, to);
		sys::del_watcher _del_watcher;
		auto _addr = _del_watcher.mutable_addr();
		*_addr = from;
		send_to_net(make_message(from, to, _del_watcher));
	}

	void engine::add_engine_watcher(addr &from, addr &to)
	{
		std::lock_guard<std::mutex> lock_guard_(engine_watcher_.locker_);
		auto &watcher_info = engine_watcher_.watchers_[to.engine_id()];
		watcher_info.actors_.insert(from);
		watcher_info.engine_id_ = to.engine_id();
		if (watcher_info.timer_id_)
			return;
		auto engine_id = to.engine_id();
		watcher_info.timer_id_= 
			timer_.set_timer(config_.net_heartbeart_interval, [=] {
			ping(engine_id);
			return true;
		});
	}


	void engine::del_engine_watcher(addr &from, addr &to)
	{
		std::lock_guard<std::mutex> lock_guard_(engine_watcher_.locker_);
		auto &watcher_info = engine_watcher_.watchers_[to.engine_id()];
		watcher_info.actors_.erase(from);
		if (watcher_info.actors_.size())
			return;
		timer_.cancel_timer(watcher_info.timer_id_);
		watcher_info.timer_id_= 0;
	}

	void engine::resp_pong(const message_base::ptr &msg)
	{
		sys::pong pong;
		pong.set_engine_id(engine_id_);
		addr from;
		from.set_engine_id(engine_id_);
		from.set_actor_id(engine_actor_id_);
		send(make_message(from, msg->from(), pong));
	}

	void engine::handle_pong(const message_base::ptr &msg)
	{
		std::lock_guard<std::mutex> lock_guard_(engine_watcher_.locker_);
		engine_watcher_.watchers_[msg->from().engine_id()].last_pong_ = 
			high_resolution_clock::now();
	}

	void engine::check_engine_watcher()
	{
		std::list<engine_watcher::watcher> timeout_list_;
		auto now = high_resolution_clock::now();
		do 
		{
			auto timeout = config_.net_heartbeart_timeout;
			std::lock_guard<std::mutex> lock_guard_(engine_watcher_.locker_);
			for (auto itr = engine_watcher_.watchers_.begin();
				itr != engine_watcher_.watchers_.end();)
			{
				if (std::chrono::duration_cast<std::chrono::milliseconds>
					(now - itr->second.last_pong_).count() > timeout)
				{
					timeout_list_.push_back(itr->second);
					if (itr->second.timer_id_)
					{
						timer_.cancel_timer(itr->second.timer_id_);
					}
				}
			}

		} while (false);

		for (auto &itr: timeout_list_)
		{
			for (auto &to : itr.actors_)
			{
				sys::engine_offline offline;
				addr from;
				from.set_engine_id(itr.engine_id_);
				offline.set_engine_id(itr.engine_id_);
				send(make_message(from, to, offline));
			}
		}
	}

	void engine::ping(uint64_t to_engine_id)
	{
		sys::ping ping;
		ping.set_engine_id(engine_id_);
		addr from;
		addr to;
		from.set_engine_id(engine_id_);
		from.set_actor_id(engine_actor_id_);

		to.set_engine_id(to_engine_id);
		to.set_actor_id(engine_actor_id_);

		auto msg = make_message(from, to, ping);
		send_to_net(msg);
	}

	void engine::regist_engine()
	{
		std::promise<nameserver::regist_engine_resp> resp_promise;
		auto futrue = resp_promise.get_future();
		auto type = get_message_type<nameserver::regist_engine_resp>();

		regist_msg_handle(type,[&](const message_base::ptr &ptr) 
		{
			try
			{
				auto msg = ptr->get<nameserver::regist_engine_resp>();
				if (!msg)
					throw std::runtime_error("get<sys::regsit_engine_resp> nullptr");

				resp_promise.set_value(*msg);
			}
			catch (...)
			{
				resp_promise.set_exception(std::current_exception());
			}
		});
		
		nameserver::regist_engine_req req;
		req.mutable_engine_info()->set_net_addr(config_.net_bind_addr_);
		req.mutable_engine_info()->set_engine_id(0);
		req.mutable_engine_info()->set_engine_name(config_.engine_name_);

		send(make_message(engine_addr_, nameserver_leader_, req));

		try
		{
			futrue.wait();
			auto resp = futrue.get();
			if (!resp.result())
				throw std::runtime_error("regist_engine_req failed");
			engine_id_ = resp.engine_id();
			std::cout << "regist_engine ok. engine_id " << engine_id_ << std::endl;
		}
		catch (std::exception&e)
		{
			std::cout << e.what() << std::endl;
			unregist_msg_handle(get_message_type<nameserver::regist_engine_resp>());
			throw;
		}
		unregist_msg_handle(get_message_type<nameserver::regist_engine_resp>());
	}

	void engine::init_nameserver_cluster()
	{
		nameserver_info_.clear();
		init_nameserver_info(default_nameserver_);
		find_nameserver_leader();
	}

	void engine::init_nameserver_info(addr nameserver_addr)
	{
		std::promise<nameserver::get_cluster_list_resp> resp_promise;
		auto future = resp_promise.get_future();
		auto type = get_message_type<nameserver::get_cluster_list_resp>();
		auto req_id = req_id_++;

		regist_msg_handle(type, [&](const message_base::ptr &ptr) 
		{
			try
			{
				auto msg = ptr->get<nameserver::get_cluster_list_resp>();
				if (!msg)
					throw std::runtime_error("get<sys::get_engine_list_resp> nullptr");
				if (msg->req().req_id() != req_id)
				{
					return;
				}
				resp_promise.set_value(*msg);
			}
			catch (...)
			{
				resp_promise.set_exception(std::current_exception());
			}
		});

		connect(config_.nameserver_engine_id_, config_.nameserver_addr_);

		nameserver::get_cluster_list_req req;

		req.set_req_id(req_id);
		send(make_message(engine_addr_, nameserver_addr, req));

		auto resp = future.get();
		for (int i = 0; i < resp.nameserver_info_size();i++)
		{
			auto item =  resp.nameserver_info(i);
			nameserver_info_.emplace(item.addr(), item);
		}
	}

	void engine::connect_nameserver()
	{
		for (auto &itr : nameserver_info_)
		{
			connect(itr.first.engine_id(), itr.second.net_addr());
		}
	}

	void engine::find_nameserver_leader()
	{
		for (auto &itr : nameserver_info_)
		{
			nameserver::check_leader req;
			std::promise<nameserver::check_leader_result> resp_promise;
			auto future = resp_promise.get_future();
			auto req_id = gen_req_id();
			auto type = get_message_type<nameserver::check_leader_result>();
			regist_msg_handle(type, [&](const message_base::ptr &ptr) {
				try
				{
					auto msg = ptr->get<nameserver::check_leader_result>();
					if (!msg)
						throw std::runtime_error("get<nameserver::check_leader_result> nullptr");
					if (msg->req_id() != req_id)
					{
						return;
					}
					resp_promise.set_value(*msg);
				}
				catch (...)
				{
					resp_promise.set_exception(std::current_exception());
				}
			});

			req.set_req_id(req_id);
			send(make_message(engine_addr_, itr.first, req));
			auto timeout = future.wait_for(std::chrono::milliseconds(3000));
			if (timeout == std::future_status::timeout)
				continue;

			auto resp = future.get();
			if (resp.is_leader())
			{
				nameserver_leader_ = itr.first;
				return;
			}
		}
		throw std::runtime_error("can't find nameserver leader");
	}

	engine::msg_process_handle engine::find_msg_handle(std::string &type)
	{
		auto itr = msg_handles_.find(type);
		if (itr != msg_handles_.end())
			return itr->second;
		return nullptr;
	}

	void engine::regist_msg_handle(std::string &type, const msg_process_handle &handle)
	{
		msg_handles_[type] = handle;
	}

	void engine::unregist_msg_handle(std::string &type)
	{
		msg_handles_.erase(type);
	}

	uint64_t engine::gen_req_id()
	{
		return req_id_++;
	}

}