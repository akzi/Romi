#pragma once
namespace romi
{
#define REGIST_MESSAGE_BUILDER(TYPE)\
	message_builder::instance().\
	regist(get_message_type<TYPE>(),\
	[](const void *buffer, std::size_t len) {\
		return message<TYPE>::parse_from_array(buffer, len);\
	});

#define REGIST_RECEIVE(MESSAGE) \
		actor::receive([this] (const romi::addr &addr, const MESSAGE &message)\
		{return receive(addr, message);});

	template<typename T>
	inline std::enable_if_t<std::is_base_of<
		::google::protobuf::Message, T>::value, std::string>
		get_message_type()
	{
		return T::descriptor()->full_name();
	}

	template<typename T>
	inline std::enable_if_t<std::is_base_of<
		::google::protobuf::Message, T>::value, std::string>
		serialize_to_string(const T &obj)
	{
		std::string buffer;
		if (!obj.SerializeToString(&buffer))
		{
			throw std::runtime_error("obj.SerializeToString error");
		}
		return buffer;
	}
	inline  std::size_t get_sizeof(addr)
	{
		return sizeof(uint64_t) + sizeof(uint64_t);
	}

	inline std::size_t get_sizeof(const std::string &str)
	{
		return sizeof(uint32_t) + str.size();
	}

	inline void encode_uint32(uint8_t *&buffer_, uint32_t value)
	{
		buffer_[0] = (unsigned char)(((value) >> 24) & 0xff);
		buffer_[1] = (unsigned char)(((value) >> 16) & 0xff);
		buffer_[2] = (unsigned char)(((value) >> 8) & 0xff);
		buffer_[3] = (unsigned char)(value & 0xff);
		buffer_ += sizeof(value);
	}

	inline void encode_uint32(std::ofstream &file, uint32_t value)
	{
		uint8_t buffer[sizeof(uint32_t)];
		uint8_t *ptr = buffer;
		encode_uint32(ptr, value);
		file.write((char*)buffer, sizeof(uint32_t));
	}

	inline uint32_t decode_uint32(uint8_t *&buffer_)
	{
		uint32_t value =
			(((uint32_t)buffer_[0]) << 24) |
			(((uint32_t)buffer_[1]) << 16) |
			(((uint32_t)buffer_[2]) << 8) |
			((uint32_t)buffer_[3]);
		buffer_ += sizeof(value);
		return value;
	}


	inline void encode_uint64(uint8_t *&buffer_, uint64_t value)
	{
		buffer_[0] = (unsigned char)(((value) >> 56) & 0xff);
		buffer_[1] = (unsigned char)(((value) >> 48) & 0xff);
		buffer_[2] = (unsigned char)(((value) >> 40) & 0xff);
		buffer_[3] = (unsigned char)(((value) >> 32) & 0xff);
		buffer_[4] = (unsigned char)(((value) >> 24) & 0xff);
		buffer_[5] = (unsigned char)(((value) >> 16) & 0xff);
		buffer_[6] = (unsigned char)(((value) >> 8) & 0xff);
		buffer_[7] = (unsigned char)(value & 0xff);
		buffer_ += sizeof(uint64_t);
	}

	inline void encode_uint64(std::ofstream &file, uint64_t value)
	{
		uint8_t buffer[sizeof(uint64_t)];
		uint8_t *ptr = buffer;
		encode_uint64(ptr, value);
		file.write((char*)buffer, sizeof(uint64_t));
	}
	inline void encode_uint8(std::string &buffer, uint8_t value)
	{
		buffer.push_back((char)value);
	}
	inline void encode_uint64(std::string &buffer, uint64_t value)
	{
		uint8_t uint64_buffer[sizeof(uint64_t)];
		uint8_t *ptr = uint64_buffer;
		encode_uint64(ptr, value);

		buffer.append((char*)uint64_buffer, sizeof(value));
	}

	inline uint8_t decode_uint8(std::ifstream &file)
	{
		uint8_t value = 0;
		file.read((char*)value, sizeof(value));
		return value;
	}
	

	

	inline uint64_t decode_uint64(uint8_t *&buffer_)
	{
		uint64_t value =
			(((uint64_t)buffer_[0]) << 56) |
			(((uint64_t)buffer_[1]) << 48) |
			(((uint64_t)buffer_[2]) << 40) |
			(((uint64_t)buffer_[3]) << 32) |
			(((uint64_t)buffer_[4]) << 24) |
			(((uint64_t)buffer_[5]) << 16) |
			(((uint64_t)buffer_[6]) << 8) |
			((uint64_t)buffer_[7]);
		buffer_ += sizeof(uint64_t);
		return value;
	}
	inline uint64_t decode_uint64(std::ifstream &file)
	{
		uint8_t uint64[sizeof(uint64_t)];
		uint8_t *ptr = uint64;
		file.read((char*)uint64, sizeof(uint64_t));
		return decode_uint64(ptr);
	}

	inline void encode_string(uint8_t *&buffer_, const std::string &str)
	{
		encode_uint32(buffer_, (uint32_t)str.size());
		memcpy(buffer_, str.data(), str.size());
		buffer_ += str.size();
	}

	inline void encode_string(std::ofstream &file, const std::string &str)
	{
		std::string buffer;
		buffer.resize(get_sizeof(str));
		uint8_t *ptr = (uint8_t *)buffer.data();
		encode_string(ptr, str);

		file.write(buffer.data(), buffer.size());
	}

	inline void encode_string(std::string &buffer, const std::string &str)
	{
		uint8_t len[sizeof(uint32_t)];
		uint8_t *ptr = len;
		encode_uint32(ptr, (uint32_t)str.size());
		buffer.append((char*)len, sizeof(len));
		buffer.append(str);
	}

	inline std::string decode_string(std::ifstream &file)
	{
		auto len = decode_uint64(file);
		std::string buffer;
		buffer.resize(len);

		file.read((char *)buffer.data(), buffer.size());
		return buffer;
	}

	inline std::string decode_string(uint8_t *&buffer_)
	{
		auto len = decode_uint32(buffer_);
		assert(len >= 0);
		std::string value;
		value.append((char*)buffer_, len);
		buffer_ += len;
		return value;
	}




	//
	inline google::protobuf::Message*
		build_message(const std::string& message_type, 
			const char *buffer, std::size_t len)
	{
		google::protobuf::Message* message = NULL;
		auto descriptor =
			google::protobuf::DescriptorPool::generated_pool()->
			FindMessageTypeByName(message_type);
		if (descriptor)
		{
			const auto prototype =
				google::protobuf::MessageFactory::
				generated_factory()->GetPrototype(descriptor);
			if (prototype)
			{
				message = prototype->New();
				message->ParseFromArray(buffer, (int)len);
			}
		}
		return message;
	}

	inline std::string pack_message(const google::protobuf::Message &message)
	{
		auto req_buffer = message.SerializeAsString();
		auto req_name = message.GetDescriptor()->full_name();
		std::string buffer;
		buffer.reserve(get_sizeof(req_buffer) + get_sizeof(req_name));
		encode_string(buffer, req_name);
		encode_string(buffer, req_buffer);
		return buffer;
	}

	//addr
	inline bool operator == (const addr &left, const addr &right)
	{
		return left.engine_id() == right.engine_id() &&
			left.actor_id() == right.actor_id();
	}

	//message
	template<typename T>
	void* romi::message<T>::get_impl(const std::type_info& info)const
	{
		if (typeid(T) == info)
			return value_.get();
		else
			return nullptr;
	}

	template<typename T>
	std::string romi::message<T>::serialize_as_string()
	{
		std::string buffer;
		std::string from = from_.SerializeAsString();
		std::string to = to_.SerializeAsString();
		std::string value = value_->SerializeAsString();
		buffer.resize(get_sizeof(from) + 
			get_sizeof(to) + 
			get_sizeof(type_) + 
			get_sizeof(value));

		uint8_t *ptr = (uint8_t*)buffer.data();
		encode_string(ptr, type_);
		encode_string(ptr, from);
		encode_string(ptr, to);
		encode_string(ptr, value);

		return buffer;
	}

	template<typename T>
	message_base::ptr message<T>::parse_from_array(const void* data, std::size_t len)
	{
		uint8_t *ptr = (uint8_t*)data;
		auto msg = std::make_shared<message<T>>();
		msg->type_ = decode_string(ptr);
		auto from = decode_string(ptr);
		msg->from_.ParseFromArray(from.data(), (int)from.size());
		auto to = decode_string(ptr);
		msg->to_.ParseFromArray(to.data(), (int)to.size());
		auto value = decode_string(ptr);
		auto message = build_message(msg->type_, value.data(), value.size());
		assert(message);
		msg->value_.reset(reinterpret_cast<T*>(message));
		return msg;
	}

	template<typename T>
	inline  std::shared_ptr<romi::message_base>
		make_message(const addr &from, const addr &to, const T &val)
	{
		return std::make_shared<message<std::decay_t<T>>>(from, to, val);
	}

	template<typename T>
	inline romi::message<T>::message(const addr &from, const addr &to, const T &value)
		: value_(new T(value))//copy
	{
		from_ = from;
		to_ = to;
		type_ = get_message_type<T>();
	}

	//actor
	
	template<typename Actor, typename ...Args>
	inline std::enable_if_t<std::is_base_of<actor, Actor>::value, addr>
		actor::spawn(Args &&...args)
	{
		return engine_->spawn(std::forward<Args>(args)...);
	}

	template<typename T>
	inline void actor::send(const addr &to, T &&obj)
	{
		send_msg_(make_message(addr_, to, std::forward<T>(obj)));
	}

	template<typename Handle>
	inline void actor::receive(Handle handle)
	{
		receive_help(to_function(std::forward<Handle>(handle)));
	}

	template<typename Message>
	inline void actor::receive_help(
		std::function<void(const addr&, const Message &)> handle)
	{
		auto func = [handle](message_base::ptr msg) {
			Message *message = msg->get<Message>();
			assert(message);
			handle(msg->from_, *message);
		};
		msg_handles_[get_message_type<Message>()] = func;
		REGIST_MESSAGE_BUILDER(Message);
	}
	//engine
	template<typename Actor, typename ...Args>
	inline std::enable_if_t<std::is_base_of<actor, Actor>::value, addr>
		engine::spawn(Args &&...args)
	{
		assert(is_start_);
		actor::ptr _actor = std::make_shared<Actor>(std::forward<Args>(args)...);
		init_actor(_actor);
		return _actor->addr_;
	}

	template<typename T>
	std::enable_if_t<std::is_base_of<::google::protobuf::Message, T>::value>
		engine::send(const addr &from, const addr &to, const T &msg)
	{
		assert(is_start_);
		send(make_message(from, to, msg));
	}

	//threadpool
	template<typename T>
	void threadpool::add_job(T &&job)
	{
		index_++;
		get_worker().add_job(std::forward<T>(job));
	}
}