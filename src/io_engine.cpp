#include "romi.hpp"
#include "zmq.h"

#define EVENT_TO_STR(EVENT,value)\
		if (EVENT == value)\
			return #EVENT;

namespace romi
{
namespace net
{
	command & command::operator=(command &&other)
	{
		if (&other == this)
			return *this;
		reset();
		type_ = other.type_;
		net_connect_ = other.net_connect_;
		send_msg_ = other.send_msg_;
		other.type_ = command::e_null;
		return *this;
	}

	command::command(command &&other)
	{
		*this = std::move(other);
	}

	command::command()
	{
		memset(this, 0, sizeof(command));
	}

	command::~command()
	{
		reset();
	}


	void command::reset()
	{
		switch (type_)
		{
		case command::e_null:
			break;
		case command::e_net_connect:
			delete net_connect_;
			break;
		case command::e_send_msg:
			delete send_msg_;
			break;
		default:
			break;
		}
	}

	//msg_queue
	cmd_queue::cmd_queue()
	{

	}

	void cmd_queue::init(void *zmq_ctx_, const char *addr_)
	{
		socket_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
		if (zmq_connect(socket_, addr_) == -1)
			throw std::runtime_error(zmq_strerror(errno));
	}

	void cmd_queue::push_back(command &&_msg)
	{
		if (msg_queue_.push(std::forward<command>(_msg)) == 1)
		{
			notify();
		}
	}

	bool cmd_queue::pop(command &_msg)
	{
		return msg_queue_.pop(_msg);
	}

	void cmd_queue::notify()
	{
		std::lock_guard<std::mutex> locker(mutex_);
		if (zmq_send(socket_, "K", 1, 0) == -1)
		{
			throw std::runtime_error(zmq_strerror(errno));
		}
	}

	//io_engine
	io_engine::io_engine()
	{
		zmq_ctx_ = zmq_init(1);
	}


	io_engine::~io_engine()
	{
		zmq_term(zmq_ctx_);
	}

	void io_engine::bind(const std::string &addr)
	{
		bind_addr_ = addr;
	}

	void io_engine::bind_send_msg(
		std::function<void(message_base::ptr&&)> handle)
	{
		send_msg_ = handle;
	}

	void io_engine::bind_handle_net_msg(
		std::function<void(void*, std::size_t)> handle)
	{
		handle_msg_ = handle;
	}

	void io_engine::send_cmd(command &&msg_)
	{
		msg_queue_.push_back(std::move(msg_));
	}

	void io_engine::start()
	{
		const char *addr = "inproc://io_engine.recevicer";
		std::mutex mutex_;
		std::condition_variable cv_;

		std::unique_lock<std::mutex> locker(mutex_);

		recevicer_ = std::thread([&] {
			start_recevicer([&] {
				cv_.notify_one();
			}, addr);
		});
		cv_.wait(locker);

		monitor_ = std::thread([&] {
			start_monitor([&] {
				cv_.notify_one();
			}, addr);
		});
		cv_.wait(locker);

		sender_ = std::thread([&] {
			start_sender([&] {
				cv_.notify_one();
			});
		});
		cv_.wait(locker);
	}

	void io_engine::stop()
	{
		is_stop_ = true;
		recevicer_.join();
		sender_.join();
	}
	void io_engine::start_recevicer(std::function<void()> init_done, const char *addr)
	{
		auto socket = zmq_socket(zmq_ctx_, ZMQ_PULL);
		if (!socket)
			throw std::runtime_error(zmq_strerror(errno));

		if (zmq_bind(socket, bind_addr_.c_str()) == -1)
		{
			zmq_close(socket);
			throw std::runtime_error(zmq_strerror(errno));
		}
		if (zmq_socket_monitor(socket, addr, ZMQ_EVENT_ALL) == -1)
		{
			zmq_close(socket);
			throw std::runtime_error(zmq_strerror(errno));
		}
		int val = 100;
		if (zmq_setsockopt(socket, ZMQ_RCVTIMEO, &val, sizeof(val)) == -1)
		{
			zmq_close(socket);
			throw std::runtime_error("zmq_setsockopt failed");
		}

		init_done();
		zmq_msg_t msg;
		zmq_msg_init(&msg);
		while (is_stop_ == false)
		{
			if (zmq_recvmsg(socket, &msg, 0) == -1)
			{
				if (errno == EAGAIN)
					continue;
				std::cout << zmq_strerror(errno) << std::endl;
			}
			try
			{
				handle_msg_(zmq_msg_data(&msg), zmq_msg_size(&msg));
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
		}
		zmq_msg_close(&msg);
		zmq_close(zmq_ctx_);
	}


	void io_engine::start_sender(std::function<void()> init_done)
	{
		const char * addr = "inproc://msg_queue_";
		int val = 100;

		auto socket = zmq_socket(zmq_ctx_, ZMQ_PAIR);
		if (zmq_bind(socket, addr) == -1)
			throw std::runtime_error(zmq_strerror(errno));
		msg_queue_.init(zmq_ctx_, addr);

		
		int rc = zmq_setsockopt(socket, ZMQ_RCVTIMEO, &val, sizeof(val));
		if (rc == -1)
			throw std::runtime_error("zmq_setsockopt failed");
		
		init_done();

		zmq_msg_t msg;
		zmq_msg_init(&msg);

		while (is_stop_ == false)
		{
			int rc = zmq_recvmsg(socket, &msg, 0);
			if (rc == -1)
			{
				if (errno == EAGAIN)
					continue;
				throw std::runtime_error(zmq_strerror(errno));
			}
			process_msg();
		}
		zmq_msg_close(&msg);
		zmq_close(socket);
	}

	static bool read_msg(void* socket, zmq_event_t& event, std::string& addr)
	{
		int rc;
		zmq_msg_t msg1;  
		zmq_msg_t msg2;  

		zmq_msg_init(&msg1);
		zmq_msg_init(&msg2);

		rc = zmq_msg_recv(&msg1, socket, 0);
		if (rc == -1 && zmq_errno() == ETERM)
			return false;

		assert(rc != -1);
		assert(zmq_msg_more(&msg1) != 0);
		rc = zmq_msg_recv(&msg2, socket, 0);
		if (rc == -1 && zmq_errno() == ETERM)
			return false;

		assert(rc != -1);
		assert(zmq_msg_more(&msg2) == 0);
		const char* data = (char*)zmq_msg_data(&msg1);
		memcpy(&event.event, data, sizeof(event.event));
		memcpy(&event.value, data + sizeof(event.event), sizeof(event.value));
		// copy address part
		addr = std::string((char*)zmq_msg_data(&msg2), zmq_msg_size(&msg2));

		if (event.event == ZMQ_EVENT_MONITOR_STOPPED)
			return false;

		return true;
	}

	const char *event_to_str(int event)
	{

		EVENT_TO_STR(ZMQ_EVENT_CONNECTED, event);
		EVENT_TO_STR(ZMQ_EVENT_CONNECT_DELAYED, event);
		EVENT_TO_STR(ZMQ_EVENT_CONNECT_RETRIED, event);
		EVENT_TO_STR(ZMQ_EVENT_LISTENING, event);
		EVENT_TO_STR(ZMQ_EVENT_BIND_FAILED, event);
		EVENT_TO_STR(ZMQ_EVENT_ACCEPTED, event);
		EVENT_TO_STR(ZMQ_EVENT_ACCEPT_FAILED, event);
		EVENT_TO_STR(ZMQ_EVENT_CLOSED, event);
		EVENT_TO_STR(ZMQ_EVENT_CLOSE_FAILED, event);
		EVENT_TO_STR(ZMQ_EVENT_DISCONNECTED, event);
		EVENT_TO_STR(ZMQ_EVENT_MONITOR_STOPPED, event);
		
		return "";
	}
	void io_engine::start_monitor(std::function<void()> init_done, const char *addr_)
	{
		auto socket = zmq_socket(zmq_ctx_, ZMQ_PAIR);
		assert(socket);
		if (zmq_connect(socket, addr_) == -1)
		{
			zmq_close(socket);
			throw std::runtime_error(zmq_strerror(errno));
		}
		init_done();
		zmq_event_t event;
		std::string remote_addr;
		while (is_stop_ == false )
		{
			if (read_msg(socket, event, remote_addr) == false)
				break;
			std::string info = remote_addr + " "+ event_to_str(event.event) \
				+ " "+ std::to_string(event.value);
			std::cout << info << std::endl;
		}
	}

	io_engine::socket io_engine::connect(std::string remote_addr)
	{
		auto socket_ = zmq_socket(zmq_ctx_, ZMQ_PUSH);
		if (!socket_)
			throw std::runtime_error(zmq_strerror(errno));
		if (zmq_connect(socket_, remote_addr.c_str()) == -1)
		{
			zmq_close(socket_);
			return nullptr;
		}
		return socket_;
	}

	void io_engine::do_connect(command &_msg)
	{
		auto engine_id = _msg.net_connect_->engine_id();
		auto remote_addr = _msg.net_connect_->remote_addr();
		addr to = _msg.net_connect_->from();

		sys::net_connect_notify notify;
		notify.set_connected(true);
		notify.mutable_net_connect()->CopyFrom(*_msg.net_connect_);

		std::shared_ptr<socket_info::socket> socket;
		 for (auto &itr: sockets_)
		 {
			 if (itr.second.remote_addr_ == remote_addr)
			 {
				 socket = itr.second.socket_;
				 break;
			 }
		 }
		 if (!socket)
		 {
			 auto sock = connect(remote_addr);
			 if (!sock)
				 return send_msg_(make_message(to, to, notify));

			 socket.reset(new socket_info::socket (sock), 
				 [](socket_info::socket * socket){
				 zmq_close(socket->socket_);
			 });
		 }
		if (socket)
		{
			sockets_[engine_id] = {socket, remote_addr};
			notify.set_connected(true);
			send_msg_(make_message(to, to, notify));
		}
	}

	void io_engine::do_send(command &_msg)
	{
		auto &message = _msg.send_msg_->message_;
		uint64_t engine_id = message->to().engine_id();
		const auto itr = sockets_.find(engine_id);
		if (itr == sockets_.end())
		{
			sys::net_not_engine_id notify;

			notify.mutable_addr()->CopyFrom(message->to());
			send_msg_(make_message(message->from(), message->from(), notify));
			return;
		}
		auto socket = itr->second.socket_->socket_;
		auto buffer = message->serialize_as_string();

		if (zmq_send(socket, buffer.data(), buffer.size(), 0) == -1)
			std::cout << zmq_strerror(errno) << std::endl;
	}

	void io_engine::process_msg()
	{
		while (true)
		{
			command cmd;
			if (!msg_queue_.pop(cmd))
				return;
			switch (cmd.type_)
			{
			case command::e_net_connect:
				do_connect(std::move(cmd));
				break;
			case command::e_send_msg:
				do_send(std::move(cmd));
				break;
			default:
				break;
			}
		}
	}
}
}

