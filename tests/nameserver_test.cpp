#include "romi.hpp"

struct nameserver_test: romi::actor 
{
private:
	virtual void init() override
	{
		using namespace romi;
		
		receive([this](const addr &from, const romi::nameserver::get_engine_list_resp &resp) {

			for (int i = 0; i < resp.engine_info_size(); i++)
			{
				auto engine_info = resp.engine_info(i);
				std::cout << "addr:"<<engine_info.net_addr() << std::endl;
				std::cout << "engine_id:"<<engine_info.engine_id() << std::endl;
				std::cout << "name:"<<engine_info.engine_name() << std::endl;
			}
		});
		send(get_nameserver_addr(), romi::nameserver::get_engine_list_req{});

		std::cout << "dispatcher count:" << get_dispatcher_size() << std::endl;
		std::cout << "actor count:" << get_actor_size() << std::endl;

		increase_dispather(10);
		std::cout << "dispatcher count:" << get_dispatcher_size() << std::endl;
	}
};

int main()
{
	romi::engine engine;


	romi::config cfg;
	cfg.engine_name_ = "nameserver_test";

	engine.set_config(cfg);

	engine.start();
	for (int i = 0; i < 1000; i++)
	{
		engine.add_job([i] {
			std::cout << i << std::endl;
		});
	}
	

	engine.spawn<nameserver_test>();

	getchar();

	engine.stop();

	return 0;
}