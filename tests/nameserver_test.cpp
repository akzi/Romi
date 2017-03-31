#include "romi.hpp"

struct nameserver_test: romi::actor 
{
private:
	virtual void init() override
	{
		using namespace romi;
		
		receivce([this](const addr &from, const sys::get_engine_list_resp &resp) {

			for (int i = 0; i < resp.engine_info_size(); i++)
			{
				auto engine_info = resp.engine_info(i);
				std::cout << "addr:"<<engine_info.addr() << std::endl;
				std::cout << "engine_id:"<<engine_info.engine_id() << std::endl;
				std::cout << "name:"<<engine_info.name() << std::endl;
			}
		});

		send(get_nameserver_addr(), sys::get_engine_list_req{});
	}
};

int main()
{
	romi::engine engine;

	romi::config cfg;
	cfg.engine_name_ = "nameserver_test";

	engine.set_config(cfg);

	engine.start();

	engine.spawn<nameserver_test>();

	getchar();


}