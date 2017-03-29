#include "romi.hpp"
#include "nameservice.h"


int main()
{
	romi::engine engine;
	engine.start();
	engine.spawn<romi::nameservice>();
}