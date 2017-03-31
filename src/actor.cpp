#include "romi.hpp"
namespace romi
{


	actor::actor()
	{
		init_msg_process_handle();
	}

	actor::~actor() 
	{
	
	}

	void actor::send(message_base::ptr &&msg)
	{
		send_msg_(std::move(msg));
	}

	timer_id actor::set_timer(std::size_t mills, timer_handle &&handle)
	{
		timer_handles_[++timer_id_] = { 0, handle };
		timer_handles_[timer_id_].first = set_timer_(addr_, mills, timer_id_);
		return timer_id_;
	}

	void actor::cancel_timer(timer_id id)
	{
		auto itr = timer_handles_.find(id);
		if (itr != timer_handles_.end())
		{
			cancel_timer_(itr->second.first);
			timer_handles_.erase(itr);
		}
	}

	void actor::watch(addr actor_)
	{
		if (actor_ == addr_ ||
			watchers_.find(actor_) != watchers_.end())
			return;
		watchers_.insert(actor_);
		add_watcher_(addr_, actor_);
	}


	void actor::cancel_watch(addr actor_)
	{
		if (actor_ == addr_ ||
			watchers_.find(actor_) == watchers_.end())
			return;
		watchers_.erase(actor_);
		cancel_watch_(addr_, actor_);
	}

	void actor::close()
	{
		for (auto itr: observers_)
		{
			sys::actor_close _actor_close;
			actor_info info;
			_actor_close.mutable_addr()->CopyFrom(addr_);
			send(itr, _actor_close);
		}
		for (auto itr :watchers_)
		{
			cancel_watch_(addr_, itr);
		}
		watchers_.clear();
		observers_.clear();
		is_close_ = true;
	}


	romi::addr actor::get_addr()
	{
		return addr_;
	}


	romi::addr actor::get_engine_addr()
	{
		addr engine_addr;
		engine_addr.set_engine_id(addr_.engine_id());
		engine_addr.set_actor_id(0);
		return engine_addr;
	}

	void actor::connect(sys::net_connect &msg)
	{
		send(get_engine_addr(), msg);
	}

	void actor::init()
	{
		std::cout << "actor init" << std::endl;
	}

	bool actor::dispatch_msg()
	{
		message_base::ptr msg;
		if (msg_queue_.pop(msg))
		{
			apply_msg(msg);
		}

		if (is_close_)
		{
			close_callback_(addr_);
			return false;
		}
		return !!msg_queue_.jobs();
	}

	void actor::apply_msg(const message_base::ptr &msg)
	{
		auto &handle = msg_handles_[msg->type()];
		if (!handle)
		{
			std::cout << "can't find msg handle; "
				<< msg->type() << std::endl;
			return;
		}
		try
		{
			handle(msg);
		}
		catch (std::exception &e)
		{
			std::cout << e.what() << std::endl;
			close();
		}
		catch (...)
		{
			std::cout << "catch exception ,close "<< std::endl;
			close();
		}
	}

	void actor::timer_expire(timer_id id)
	{
		auto itr = timer_handles_.find(id);
		if (itr == timer_handles_.end() || itr->second.second())
			return;
		cancel_timer_(itr->second.first);
		timer_handles_.erase(itr);
	}

	std::size_t actor::receive_msg(message_base::ptr &&msg)
	{
		return msg_queue_.push(std::move(msg));
	}

	void actor::init_msg_process_handle()
	{
		msg_handles_.emplace(get_message_type<sys::timer_expire>(),
			[this](const message_base::ptr& msg) {
			if (const auto ptr = msg->get<sys::timer_expire>())
			{
				timer_expire(ptr->timer_id());
			}
		});

		msg_handles_.emplace(get_message_type<sys::add_watcher>(),
			[this](const message_base::ptr& msg) {

			if (const auto ptr = msg->get<sys::add_watcher>())
			{
				observers_.insert(ptr->addr());
			}
		});

		msg_handles_.emplace(get_message_type<sys::del_watcher>(), 
			[this](const message_base::ptr& msg) {

			if (const auto ptr = msg->get<sys::del_watcher>())
			{
				observers_.erase(ptr->addr());
			}
		});

		msg_handles_.emplace(get_message_type<sys::engine_offline>(), 
			[this](const message_base::ptr& msg) {

			auto &handle = msg_handles_[get_message_type<sys::actor_close>()];
			if (!handle)
				throw std::runtime_error("not find sys::actor_close msg handle");

			if (const auto ptr = msg->get<sys::engine_offline>())
			{
				sys::actor_close event;
				for (auto &itr: watchers_)
				{
					if (itr.engine_id() == ptr->engine_id())
					{
						event.mutable_addr()->CopyFrom(itr);
						handle(make_message(itr, addr_, event));
					}
				}
			}
		});
		msg_handles_.emplace(get_message_type<sys::actor_init>(),
			[this](const message_base::ptr& msg) {

			if (msg->get<sys::actor_init>())
			{
				init();
			}
		});
	}

}