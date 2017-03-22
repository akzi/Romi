#include "romi.hpp"
namespace romi
{


	actor::actor()
	{
		msg_queue_.check_read();
	}

	actor::~actor() {}


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
	void actor::add_watcher(const event::add_actor_watcher &watcher)
	{
		if (watcher.actor_ == addr_ ||
			actors_watchers_.find(watcher.actor_) != actors_watchers_.end())
			return;
		add_actor_watcher_(addr_, watcher);
	}
	void actor::add_watcher(const event::add_engine_watcher& watcher)
	{
		if (watcher.engid_id_ == addr_.engine_id_||
			engine_watchers_.find(watcher.engid_id_) == engine_watchers_.end())
			return;
		add_engine_watcher_(addr_, watcher);
	}
	void actor::del_watcher(const event::del_actor_watcher& watcher)
	{
		if (watcher.actor_ == addr_ ||
			actors_watchers_.find(watcher.actor_) == actors_watchers_.end())
			return;
		actors_watchers_.erase(watcher.actor_);
		del_actor_watcher_(addr_, watcher);
	}
	void actor::del_watcher(const event::del_engine_watcher& watcher)
	{
		if (watcher.engid_id_ == addr_.engine_id_ ||
			engine_watchers_.find(watcher.engid_id_) == engine_watchers_.end())
			return;
		engine_watchers_.erase(watcher.engid_id_);
		del_engine_watcher_(addr_, watcher);
	}
	void actor::close()
	{

	}

	void actor::init()
	{
		std::cout << "actor init" << std::endl;
	}

	bool actor::dispatch_msg()
	{
		auto item = msg_queue_.read();
		if (!item.first)
			return false;
		dispatch_msg(item.second);
		return msg_queue_.check_read();
	}

	void actor::dispatch_msg(const std::shared_ptr<message_base> &msg)
	{
		if (apply_msg(msg))
		{
			return;
		}
		else if(msg->get<sys::actor_init>())
		{
			init();
		}
		else if(const auto ptr = msg->get<sys::timer_expire>())
		{
			timer_expire(ptr->id_);
		}
		else
		{
			default_msg_process(msg);
		}
	}


	bool actor::apply_msg(const std::shared_ptr<message_base> &msg)
	{
		auto &func = msg_handles_[msg->type_];
		if (func)
		{
			try
			{
				func(msg);
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
			catch (...)
			{
				std::cout << "catch a exception" << std::endl;
			}
			return true;
		}
		return false;
	}


	void actor::default_msg_process(const std::shared_ptr<message_base> &msg)
	{
		std::cout << "Can't find message process handle : " <<
			msg->type_.c_str() << std::endl;
	}

	void actor::timer_expire(timer_id id)
	{
		auto itr = timer_handles_.find(id);
		if (itr != timer_handles_.end())
		{
			if (!itr->second.second())
			{
				cancel_timer_(itr->second.first);
				timer_handles_.erase(itr);
			}
		}
	}

	bool actor::receive_msg(std::shared_ptr<message_base> &&msg)
	{
		std::lock_guard<spinlock> lg(lock_);

		msg_queue_.write(std::move(msg));
		return msg_queue_.flush();
	}

}