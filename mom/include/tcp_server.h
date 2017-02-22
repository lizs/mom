// author : lizs
// 2017.2.22

#pragma once

#include "bull.h"
#include "scheduler.h"
#include "session_mgr.h"
#include <string>
#include <thread>

namespace Bull {
	template <typename T>
	class TcpServer {
	public:
		TcpServer(const char* ip, int port);
		int startup();
		int shutdown();
		SessionMgr<T>& get_session_mgr();

	private:
		static void connection_cb(uv_stream_t* server, int status);

		uv_tcp_t m_server;
		Scheduler m_scheduler;
		SessionMgr<T> m_sessions;

		std::string m_ip;
		int m_port;
	};

	template <typename T>
	TcpServer<T>::TcpServer(const char* ip, int port) : m_ip(ip), m_port(port) {
		m_server.data = this;
	}

	template <typename T>
	int TcpServer<T>::startup(){
		struct sockaddr_in addr;
		int r;
		uv_loop_t* loop;

		loop = uv_default_loop();
		r = uv_ip4_addr(m_ip.c_str(), m_port, &addr);
		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		r = uv_tcp_init(loop, &m_server);
		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		r = uv_tcp_bind(&m_server, reinterpret_cast<const sockaddr*>(&addr), 0);
		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		r = uv_listen(reinterpret_cast<uv_stream_t*>(&m_server), SOMAXCONN, connection_cb);
		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		LOG("Server listening on port : %d", m_port);

#if MONITOR_ENABLED
		// performance monitor
		//m_scheduler.invoke(1000, 1000, []()
		//                   {
		//	                   LOG("%llu /s", Session::get_readed());
		//	                   Session::set_readed(0);
		//                   });
#endif

		// main loop
		uv_run(loop, UV_RUN_DEFAULT);

		MAKE_VALGRIND_HAPPY();
		return 0;
	}

	template <typename T>
	int TcpServer<T>::shutdown() {
		m_sessions.close_all();
		while (m_sessions.size()) {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}

		return 0;
	}

	template <typename T>
	SessionMgr<T>& TcpServer<T>::get_session_mgr() {
		return m_sessions;
	}

	template <typename T>
	void TcpServer<T>::connection_cb(uv_stream_t* server, int status) {
		uv_stream_t* stream;
		uv_loop_t * loop;
		int r;

		LOG_UV_ERR(status);

		// make session
		auto session = new T(nullptr, [](T* session) {
								 auto host = session->get_host();
								 if(host) {
									 host->remove(session);
								 }
		                     });

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
