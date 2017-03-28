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
	std::string buffer;

	Book myBook;
	myBook.set_name("hello");
	myBook.set_pages(10);
	myBook.set_price(10);

	myBook.SerializeToString(&buffer);




	auto name = get_message_type<Book>();
	auto book = build_message2(name, buffer.data(), buffer.size());
}