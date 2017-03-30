#include <iostream>
#include "test.pb.h"
#include "romi.hpp"
using namespace romi;



google::protobuf::Message*
build_message2(const std::string& message_type, const char *buffer, std::size_t len)
{
	google::protobuf::Message* message = NULL;
	auto descriptor =
		google::protobuf::DescriptorPool::generated_pool()->
		FindMessageTypeByName(message_type);
	if (descriptor)
	{
		const auto prototype =
			google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype)
		{
			message = prototype->New();
			message->ParseFromArray(buffer, (int)len);
		}
	}
	return message;
}



int main()
{
	std::cout << sys::actor_init::descriptor()->full_name() << std::endl;

}