// author : lizs
// 2017.2.22

#pragma once

#include "bull.h"
#include "scheduler.h"
#include "session_mgr.h"

namespace Bull {
	template <typename T>
	class TcpServer {
	public:
		TcpServer();
		int start(const char* ip, int port);
		SessionMgr<T>& get_session_mgr();

	private:
		static void alloc_cb(uv_handle_t* handle,
		                     size_t suggested_size,
		                     uv_buf_t* buf);

		static void read_cb(uv_stream_t* handle,
		                    ssize_t nread,
		                    const uv_buf_t* buf);

		static void connection_cb(uv_stream_t* server, int status);

		uv_tcp_t m_server;
		Scheduler m_scheduler;
		SessionMgr<T> m_sessions;
	};

	template <typename T>
	void TcpServer<T>::alloc_cb(uv_handle_t* handle,
	                            size_t suggested_size,
	                            uv_buf_t* buf) {
		auto& cbuf = static_cast<T*>(handle->data)->get_read_cbuf();
		buf->base = cbuf.get_tail();
		buf->len = cbuf.get_writable_len();
	}

	template <typename T>
	int TcpServer<T>::start(const char* ip, int port) {
		struct sockaddr_in addr;
		int r;
		uv_loop_t* loop;

		loop = uv_default_loop();
		r = uv_ip4_addr(ip, port, &addr);
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

		LOG("Server listening on port : %d", port);

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
	SessionMgr<T>& TcpServer<T>::get_session_mgr() {
		return m_sessions;
	}

	template <typename T>
	void TcpServer<T>::connection_cb(uv_stream_t* server, int status) {
		uv_stream_t* stream;
		int r;

		LOG_UV_ERR(status);

		// make session
		auto session = new T();
		stream = reinterpret_cast<uv_stream_t*>(&session->get_stream());
		ASSERT(stream != NULL);

		r = uv_tcp_init(uv_default_loop(), reinterpret_cast<uv_tcp_t*>(stream));
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

		r = uv_read_start(stream, alloc_cb, read_cb);
		ASSERT(r == 0);
		if (r) {
			LOG_UV_ERR(r);
			return;
		}
	}

	template <typename T>
	TcpServer<T>::TcpServer() {
		m_server.data = this;
	}

	template <typename T>
	void TcpServer<T>::read_cb(uv_stream_t* handle,
	                           ssize_t nread,
	                           const uv_buf_t* buf) {
		auto session = static_cast<T*>(handle->data);

		if (nread < 0) {
			/* Error or EOF */
			//ASSERT(nread == UV_EOF);
			if (nread != UV_EOF) {
				LOG_UV_ERR(nread);
			}

			session->close();
			return;
		}

		if (nread == 0) {
			/* Everything OK, but nothing read. */
			session->close();
			return;
		}

		if (!session->dispatch(nread)) {
			session->close();
		}
	}
}