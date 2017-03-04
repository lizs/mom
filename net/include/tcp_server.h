// author : lizs
// 2017.2.22

#pragma once

#include "net.h"
#include "scheduler.h"
#include "session_mgr.h"
#include <thread>

namespace VK {
	namespace Net {
		template <typename T>
		class TcpServer final {
		public:
			TYPE_DEFINES(T)

			TcpServer(const char* ip, int port,
			          typename T::open_cb_t open_cb,
			          typename T::close_cb_t close_cb,
			          typename T::req_handler_t req_handler,
			          typename T::push_handler_t push_handler);

			bool startup();
			bool shutdown();
			SessionMgr<T>& get_session_mgr();

			req_handler_t get_req_handler();
			push_handler_t get_push_handler();
			open_cb_t get_open_cb();
			close_cb_t get_close_cb();

		private:
			static void connection_cb(uv_stream_t* server, int status);

			uv_tcp_t m_server;
			Scheduler m_scheduler;
			SessionMgr<T> m_sessions;

			req_handler_t m_reqHandler;
			push_handler_t m_pushHandler;
			open_cb_t m_openCB;
			close_cb_t m_closeCB;

			std::string m_ip;
			int m_port;
		};

		template <typename T>
		TcpServer<T>::TcpServer(const char* ip, int port,
		                        typename T::open_cb_t open_cb,
		                        typename T::close_cb_t close_cb,
		                        typename T::req_handler_t req_handler,
		                        typename T::push_handler_t push_handler) :
			m_sessions(this),
			m_reqHandler(req_handler), m_pushHandler(push_handler),
			m_openCB(open_cb), m_closeCB(close_cb), m_ip(ip), m_port(port) {
			m_server.data = this;
		}

		template <typename T>
		bool TcpServer<T>::startup() {
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

			return true;
		}

		template <typename T>
		bool TcpServer<T>::shutdown() {
			m_sessions.close_all();
			while (m_sessions.size()) {
				std::this_thread::sleep_for(std::chrono::microseconds(1));
			}

			return true;
		}

		template <typename T>
		SessionMgr<T>& TcpServer<T>::get_session_mgr() {
			return m_sessions;
		}

		template <typename T>
		typename TcpServer<T>::req_handler_t TcpServer<T>::get_req_handler() {
			return m_reqHandler;
		}

		template <typename T>
		typename TcpServer<T>::push_handler_t TcpServer<T>::get_push_handler() {
			return m_pushHandler;
		}

		template <typename T>
		typename TcpServer<T>::open_cb_t TcpServer<T>::get_open_cb() {
			return m_openCB;
		}

		template <typename T>
		typename TcpServer<T>::close_cb_t TcpServer<T>::get_close_cb() {
			return m_closeCB;
		}

		template <typename T>
		void TcpServer<T>::connection_cb(uv_stream_t* server, int status) {
			uv_stream_t* stream;
			uv_loop_t* loop;
			int r;

			LOG_UV_ERR(status);

			auto tcp_server = static_cast<TcpServer*>(server->data);

			// make session
			auto open_cb = tcp_server->get_open_cb();
			auto close_cb = tcp_server->get_close_cb();
			auto session = new T([open_cb, tcp_server](bool success, session_t* session) {
				                     tcp_server->get_session_mgr().add_session(session);
				                     if (open_cb)
					                     open_cb(true, session);
			                     },
			                     [close_cb, tcp_server](session_t* session) {
				                     if (close_cb)
					                     close_cb(session);
				                     tcp_server->get_session_mgr().remove(session);
			                     },

			                     tcp_server->get_req_handler(),
			                     tcp_server->get_push_handler());
			session->set_host(tcp_server);

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

			/* associate server with stream */
			stream->data = session;
			// 通知会话建立
			session->notify_established(true);

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
}
