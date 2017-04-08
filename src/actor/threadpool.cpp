#include "romi.hpp"

namespace romi
{

	threadpool::threadpool(int worker_size )
		:worker_size_(worker_size)
	{
		if (!worker_size_)
			worker_size_ = std::thread::hardware_concurrency();
		init();
	}

	threadpool::~threadpool()
	{
		stop();
	}
	void threadpool::stop()
	{
		for (auto &itr : workers_)
			itr->stop();
		workers_.clear();
	}

	void threadpool::init()
	{
		srand((uint32_t)time(nullptr));

		for (uint32_t i = 0; i < worker_size_; ++i)
		{
			workers_.emplace_back(new worker([this](worker::job_t &job) {
				return steal_job(job);
			}));
		}
		is_init_done_ = true;
	}

	bool threadpool::steal_job(worker::job_t &job)
	{
		if (!is_init_done_)
			return false;

		for (std::size_t i = 0; i < workers_.size(); i++)
		{
			if (workers_[rand() % workers_.size()]->steal_job(job))
			{
				return true;
			}
		}
		return false;
	}

	threadpool::worker & threadpool::get_worker()
	{
		return *workers_[index_%workers_.size()].get();
	}

	threadpool::worker::worker(const steal_job_t &handle) :steal_job_(handle)
	{
		thread_ = std::thread([this] { do_job(); });
	}

	threadpool::worker::~worker()
	{
		stop();
	}

	void threadpool::worker::add_job(job_t &&job)
	{
		job_queue_.push(std::move(job));
	}

	void threadpool::worker::add_job(const job_t &job)
	{
		job_queue_.push(job);
	}

	void threadpool::worker::stop()
	{
		is_stop_ = true;
		job_queue_.push([this] {});
		if (thread_.joinable())
			thread_.join();
	}

	bool threadpool::worker::steal_job(job_t &job)
	{
		return job_queue_.pop(job);
	}

	std::size_t threadpool::worker::jobs()
	{
		return job_queue_.jobs();
	}

	void threadpool::worker::do_job()
	{
		while (!is_stop_)
		{
			try
			{
				job_t job;
				if (job_queue_.pop(job) || steal_job_(job))
				{
					job();
					continue;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
		};		
	}
}

