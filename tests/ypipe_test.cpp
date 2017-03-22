#include "../src/romi.hpp"

using namespace romi;


void ypipe_test()
{
	ypipe<uint64_t> pipe;

	uint64_t msgs = 0;
	uint64_t last_msgs = 0;

	auto worker = std::thread([&] {
	
		uint64_t i = 0ull;
		do 
		{
			pipe.write(i++);
			pipe.flush();
		} while (1);
	});

	auto worker2 = std::thread([&] {
	
		do 
		{
			auto item = pipe.read();
			if (item.first)
				msgs = item.second;
		} while (1);
	});

	auto worker3 = std::thread([&] {

		do
		{
			std::cout << msgs - last_msgs << std::endl;
			last_msgs = msgs;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		} while (1);
	});

	getchar();
}