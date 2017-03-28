#pragma once
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
namespace romi
{

#define ROMI_DEFINE_MSG(Sym, Message) \
	template<>  struct message_traits<Message> \
	{  enum { value = 1 }; static constexpr char *_$_type = ""#Sym#Message;}

#define ROMI_DEFINE_ACTOR_MSG(Message) ROMI_DEFINE_MSG([A], Message);
#define ROMI_DEFINE_EVENT_MSG(Message) ROMI_DEFINE_MSG([E], Message);
#define ROMI_DEFINE_SYS_MSG(Message) ROMI_DEFINE_MSG([S], Message);

	template<typename T>
	struct message_traits
	{
		enum { value = 0 };
	};


	template<typename T>
	inline constexpr std::enable_if_t<message_traits<T>::value, const char *>
		get_message_type()
	{
		return message_traits<std::decay_t<T>>::_$_type;
	}

	template<typename T>
	inline std::enable_if_t<std::is_base_of<::google::protobuf::Message, T>::value, const char *>
		get_message_type()
	{
		return T::descriptor()->full_name().c_str();
	}



	template<typename T>
	inline std::enable_if<std::is_base_of<::google::protobuf::Message, T>::value, std::string> 
		serialize_to_string(T &obj)
	{
		std::string buffer;
		if (!obj.SerializeToString(buffer))
		{
			throw std::runtime_error("obj.SerializeToString error");
		}
		return buffer;
	}




	inline void encode_uint32(uint8_t *&buffer_, uint32_t value)
	{
		buffer_[0] = (unsigned char)(((value) >> 24) & 0xff);
		buffer_[1] = (unsigned char)(((value) >> 16) & 0xff);
		buffer_[2] = (unsigned char)(((value) >> 8) & 0xff);
		buffer_[3] = (unsigned char)(value & 0xff);
		buffer_ += sizeof(value);
	}

	inline uint32_t decode_uint32(const unsigned char *buffer_)
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

	inline void encode_string(uint8_t *&buffer_, std::string &str)
	{
		encode_uint32(buffer_, str.size());
		memcpy(buffer_, str.data(), str.size());
		buffer_ += str.size();
	}

	inline std::string decode_string(uint8_t *buffer_)
	{
		auto len = decode_uint32(buffer_);
		assert(len >= 0);
		std::string value;
		value.append((char*)buffer_, len);
		buffer_ += len;
		return value;
	}

	inline  std::size_t get_sizeof(addr )
	{
		return sizeof(actor_id) + sizeof(engine_id);
	}

	inline std::size_t get_sizeof(const std::string &str)
	{
		return sizeof(uint32_t) + str.size();
	}


	template<typename T>
	inline std::enable_if_t<std::is_base_of<T, google::protobuf::Message>::value, google::protobuf::Message*>
		build_message(const std::string& message_type, const char *buffer, std::size_t len)
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


	struct message_base
	{
		using ptr = std::shared_ptr<message_base>;
		addr from_;
		addr to_;
		std::string type_;
		virtual ~message_base() {}
		template<typename T> T* get();
		virtual std::string to_data() { return std::string(); }
	private:
		virtual void* get_impl(const std::type_info&info) { return nullptr; }
	};


	template<typename T>
	class message : public message_base
	{
		std::shared_ptr<T> value_;
		message() {}
	public:
		message(const addr &from, const addr &to, T &&value);
		virtual void* get_impl(const std::type_info& info);
		std::string to_data();
		static message_base::ptr make_shared(const void* buffer, std::size_t len);
	};

	

	//sys
	namespace sys
	{
		struct actor_init { };
		struct engine_offline { };
		struct timer_expire { timer_id id_; };

		struct add_watcher { addr actor_; };
		struct del_watcher { addr actor_; };

		struct actor_close { addr actor_; };

		struct not_find_remote_engine
		{
			message_base::ptr message_;
		};
		//net
		struct net_connect 
		{
			std::string remote_addr_;
			engine_id id;
			addr actor_;
		};

		struct net_connect_result
		{
			net_connect net_connect_;
			void *socket_;
		};

		struct net_close
		{
			void *socket_;
		};

		struct net_send 
		{
			std::string buffer_;
			void *socket_;
			addr from_actor_;
		};

		struct net_send_failed
		{
			net_send net_send_;
			std::string strerror_;
		};
	}
	ROMI_DEFINE_SYS_MSG(sys::engine_offline);
	ROMI_DEFINE_SYS_MSG(sys::actor_close);
	ROMI_DEFINE_SYS_MSG(sys::actor_init);
	ROMI_DEFINE_SYS_MSG(sys::timer_expire);
	ROMI_DEFINE_SYS_MSG(sys::not_find_remote_engine);
	ROMI_DEFINE_SYS_MSG(sys::net_connect_result);
	ROMI_DEFINE_SYS_MSG(sys::net_send_failed);

	ROMI_DEFINE_ACTOR_MSG(std::string);


	ROMI_DEFINE_EVENT_MSG(sys::add_watcher);
	ROMI_DEFINE_EVENT_MSG(sys::del_watcher);

	

	template<typename T>
	message_base::ptr make_message(const addr &, const addr &, T &&);

	class message_build
	{
	public:
		using build_handle =
			std::function<message_base::ptr(const void*, std::size_t)>;
		static message_build &instance()
		{
			static message_build inst;
			return inst;
		}
		void regist(std::string message_type, const build_handle &handle)
		{
			std::lock_guard<std::mutex> locker(locker_);
			handles_[message_type] = handle;
		}
		build_handle get_message_build_handle(const std::string &message_type)
		{
			std::lock_guard<std::mutex> locker(locker_);
			return handles_[message_type];
		}
	private:
		std::mutex locker_;
		std::map<std::string, build_handle> handles_;
	};
}


