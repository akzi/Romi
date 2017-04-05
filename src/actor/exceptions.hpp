#pragma once
#include <stdexcept>
namespace romi
{
	struct not_find_message_builder:std::runtime_error
	{
		not_find_message_builder(const std::string &type)
			:std::runtime_error("not_find_message_builder :"+ type)
		{
		}
	};

	struct build_message_error:std::runtime_error
	{
		build_message_error()
			:std::runtime_error("build_message_error")
		{

		}
	};
}