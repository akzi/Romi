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
			actors_watchers_.find(actor_) != actors_watchers_.end())
			return;
		actors_watchers_.insert(actor_);
		add_watcher_(addr_, actor_);
	}


	void actor::cancel_watch(addr actor_)
	{
		if (actor_ == addr_ ||
			actors_watchers_.find(actor_) == actors_watchers_.end())
			return;
		actors_watchers_.erase(actor_);
		cancel_watch_(addr_, actor_);
	}

	void actor::close()
	{
		for (auto itr: observers_)
		{
			sys::actor_close _actor_close;
			actor_info info;

			*_actor_close.mutable_addr() = addr_;
			send(itr, _actor_close);
		}
		for (auto itr :actors_watchers_)
		{
			cancel_watch_(addr_, itr);
		}
		actors_watchers_.clear();
		observers_.clear();
		is_close_ = true;
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
			dispatch_msg(msg);
		}
		return !!msg_queue_.jobs();
	}

	void actor::dispatch_msg(const message_base::ptr &msg)
	{
		for (auto &itr: default_msg_process_handles_)
		{
			if (itr(msg))
				break;
		}

		if (is_close_)
		{
			close_callback_(addr_);
		}
	}


	bool actor::apply_msg(const message_base::ptr &msg)
	{
		auto &handle = msg_handles_[msg->type_];
		if (!handle)
			return false;
		try
		{
			handle(msg);
		}
		catch (...)
		{
			std::cout << "catch a exception" << std::endl;
			close();
		}
		return true;
	}

	void actor::default_msg_process(const message_base::ptr &msg)
	{
		std::cout << "Can't find message process handle : " <<
			msg->type_.c_str() << std::endl;
	}

	void actor::timer_expire(timer_id id)
	{
		auto itr = timer_handles_.find(id);
		if (itr == timer_handles_.end() || itr->second.second())
			return;
		cancel_timer_(itr->second.first);
		timer_handles_.erase(itr);
	}

	bool actor::receive_msg(message_base::ptr &&msg)
	{
		return msg_queue_.push(std::move(msg)) == 1;
	}

	void actor::init_msg_process_handle()
	{
		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {

			return apply_msg(msg);
		});

		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {

			if (const auto ptr = msg->get<sys::timer_expire>())
			{
				timer_expire(ptr->timer_id());
				return true;
			}
			return false;
		});

		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {

			if (const auto ptr = msg->get<sys::add_watcher>())
			{
				observers_.insert(ptr->addr());
				return true;
			}
			return false;
		});

		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {

			if (const auto ptr = msg->get<sys::del_watcher>())
			{
				observers_.erase(ptr->addr());
				return true;
			}
			return false;
		});

		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {

			if (msg->get<sys::actor_init>())
			{
				init();
				return true;
			}
			return false;
		});

		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {

			default_msg_process(msg);
			return true;
		});
	}

}