#pragma once
namespace romi
{
	class threadpool
	{
	public:
		threadpool(int worker_size);
		~threadpool();
		template<typename T>
		void add_job(T &&job);
		void stop();
	private:
		class worker
		{
		public:
			using job_t = std::function<void()>;
			using steal_job_t = std::function<bool(job_t &)>;

			worker(const steal_job_t &handle);
			~worker();
			void add_job(job_t &&job);
			void add_job(const job_t &job);
			void stop();
			bool steal_job(job_t &job);
			std::size_t jobs();
		private:
			void do_job();
			bool is_stop_ = false;
			lock_queue<job_t> job_queue_;
			std::thread thread_;
			steal_job_t steal_job_;
		};
		void init();
		bool steal_job(worker::job_t &job);
		worker &get_worker();

	private:
		std::atomic<uint64_t> index_{ 0 };
		bool is_init_done_ = false;
		uint32_t worker_size_;
		std::vector<std::unique_ptr<worker>> workers_;
	};
}