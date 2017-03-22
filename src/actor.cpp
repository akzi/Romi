#include "romi.hpp"
namespace romi
{


	actor::actor()
	{
		msg_queue_.check_read();
		init_msg_process_handle();
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

	void actor::watch(addr actor_)
	{
		if (actor_ == addr_ ||
			actors_watchers_.find(actor_) != actors_watchers_.end())
			return;
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

	}

	void actor::init()
	{
		std::cout << "actor init" << std::endl;
	}

	bool actor::dispatch_msg()
	{
		lock_.lock();
		auto item = msg_queue_.read();
		lock_.unlock();
		if (!item.first || !item.second)
			return false;
		dispatch_msg(item.second);
		return msg_queue_.check_read();
	}

	void actor::dispatch_msg(const std::shared_ptr<message_base> &msg)
	{
		for (auto &itr: default_msg_process_handles_)
		{
			if (itr(msg))
				return;
		}
	}


	bool actor::apply_msg(const std::shared_ptr<message_base> &msg)
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

	void actor::default_msg_process(const std::shared_ptr<message_base> &msg)
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

	bool actor::receive_msg(std::shared_ptr<message_base> &&msg)
	{
		std::lock_guard<spinlock> lg(lock_);

		msg_queue_.write(std::move(msg));
		return msg_queue_.flush();
	}

	void actor::init_msg_process_handle()
	{
		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {

			return apply_msg(msg);
		});

		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {
			if (const auto ptr = msg->get<sys::timer_expire>())
			{
				timer_expire(ptr->id_);
				return true;
			}
			return false;
		});

		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {
			if (const auto ptr = msg->get<sys::add_watcher>())
			{
				observers_.insert(ptr->actor_);
				return true;
			}
			return false;
		});

		default_msg_process_handles_.emplace_back([this](const message_base::ptr& msg) {
			if (const auto ptr = msg->get<sys::del_watcher>())
			{
				observers_.erase(ptr->actor_);
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