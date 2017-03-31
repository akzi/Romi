#include "romi.hpp"




struct nameserver_test: romi::actor 
{
private:
	virtual void init() override
	{
		using namespace romi;
		
		std::cout << "nameserver_test" << std::endl;
		receivce([this](const addr &from, const sys::get_engine_list_resp &resp) {

			std::cout << resp.engine_info_size() << std::endl;
		});
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