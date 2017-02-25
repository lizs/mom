// author : lizs
// 2017.2.22

#pragma once

#include "bull.h"
#include "scheduler.h"
#include "session_mgr.h"
#include <string>
#include <thread>
#include "monitor.h"

namespace Bull {
	template <typename T>
	class TcpServer final {
	public:
		typedef T session_t;
		TcpServer(const char* ip, int port,
		          typename T::req_handler_t req_handler,
		          typename T::push_handler_t push_handler);
		bool startup();
		bool shutdown();
		SessionMgr<T>& get_session_mgr();

	private:
		static void connection_cb(uv_stream_t* server, int status);

		uv_tcp_t m_server;
		Scheduler m_scheduler;
		SessionMgr<T> m_sessions;
		static typename T::req_handler_t m_reqHandler;
		static typename T::push_handler_t m_pushHandler;

		std::string m_ip;
		int m_port;
	};

	template<typename T>
	typename T::req_handler_t TcpServer<T>::m_reqHandler = nullptr;
	template<typename T>
	typename T::push_handler_t TcpServer<T>::m_pushHandler = nullptr;

	template <typename T>
	TcpServer<T>::TcpServer(const char* ip, int port,
	                        typename T::req_handler_t req_handler,
	                        typename T::push_handler_t push_handler) : m_ip(ip), m_port(port) {
		m_server.data = this;
		m_reqHandler = req_handler;
		m_pushHandler = push_handler;
	}

	template <typename T>
	bool TcpServer<T>::startup(){
		struct sockaddr_in addr;
		int r;
		uv_loop_t* loop;

		loop = uv_default_loop();
		r = uv_ip4_addr(m_ip.c_str(), m_port, &addr);
		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		r = uv_tcp_init(loop, &m_server);
		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		r = uv_tcp_bind(&m_server, reinterpret_cast<const sockaddr*>(&addr), 0);
		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		r = uv_listen(reinterpret_cast<uv_stream_t*>(&m_server), SOMAXCONN, connection_cb);
		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		LOG("Server listening on port : %d", m_port);

#if MONITOR_ENABLED
		// performance monitor
		m_scheduler.invoke(1000, 1000, []() {
			                   LOG("Read : %llu /s Write : %llu", Monitor::g_readed, Monitor::g_wroted);
			                   Monitor::g_readed = 0;
			                   Monitor::g_wroted = 0;
		                   });
#endif

		return true;
	}

	template <typename T>
	bool TcpServer<T>::shutdown() {
		m_sessions.close_all();
		while (m_sessions.size()) {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}

		return true;
	}

	template <typename T>
	SessionMgr<T>& TcpServer<T>::get_session_mgr() {
		return m_sessions;
	}

	template <typename T>
	void TcpServer<T>::connection_cb(uv_stream_t* server, int status) {
		uv_stream_t* stream;
		uv_loop_t* loop;
		int r;

		LOG_UV_ERR(status);

		// make session
		auto session = new T(nullptr,
		                     [](T* session) {
			                     auto host = session->get_host();
			                     if (host) {
				                     host->remove(session);
			                     }
		                     },
		                     m_reqHandler, m_pushHandler);

		// init session
		session->prepare();

		stream = reinterpret_cast<uv_stream_t*>(&session->get_stream());
		ASSERT(stream != NULL);
		loop = uv_default_loop();
		ASSERT(loop != NULL);

		r = uv_tcp_init(loop, reinterpret_cast<uv_tcp_t*>(stream));
		ASSERT(r == 0);
		if (r) {
			delete session;
			LOG_UV_ERR(r);
			return;
		}

		auto tcp_server = static_cast<TcpServer*>(server->data);
		tcp_server->get_session_mgr().add_session(session);

		/* associate server with stream */
		stream->data = session;

		r = uv_accept(server, stream);
		ASSERT(r == 0);
		if (r) {
			LOG_UV_ERR(r);
			return;
		}

		// post first read request
		session->post_read_req();
	}
}
