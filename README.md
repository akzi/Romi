# Romi
c++ actor 
```
struct user:romi::actor
{
	user(bool val)
	{
		val_ = val;
	}
	void init()
	{
		receivce([&](const romi::addr &from, const std::string & data) {
			msgs_++;
		});
	}
	
	bool val_;
};

romi::engine engine;
	engine.start();

	std::vector<romi::addr> addrs;

	for (size_t i = 0; i < 3; i++)
	{
		addrs.push_back(engine.spawn<user>(false));
	}
	std::thread([&] {
		for (size_t i = 0; i < 1000000000; i++)
		{
			for (auto &itr : addrs)
				engine.send(itr, itr, std::string("msg"));
			std::this_thread::yield();
		}
	}).detach();
	
	std::thread([&] {
		for (size_t i = 0; i < 1000000000; i++)
		{
			for (auto &itr : addrs)
				engine.send(itr, itr, std::string("msg"));
			std::this_thread::yield();
		}
	}).detach();
	```
