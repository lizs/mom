#include "tcp_server.h"
#include "session.h"
#include <thread>
#include <vector>
#include "logger.h"

namespace VK {
	namespace Net {

		TcpServer::TcpServer(const char* ip, int port, server_handler_ptr_t handler) :
			m_sessions(this), m_handler(handler), m_ip(ip), m_port(port) {
			m_server.data = this;
		}

		bool TcpServer::startup() {
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

			Logger::instance().info("Server listening on port : {}", m_port);

			return true;
		}

		bool TcpServer::shutdown() {
			m_sessions.close_all();
			while (m_sessions.size()) {
				std::this_thread::sleep_for(std::chrono::microseconds(1));
			}

			return true;
		}

		void TcpServer::pub(const char* subject, cbuf_ptr_t pcb) {
			m_subjects.pub(subject, pcb);
		}

		void TcpServer::sub(const char* subject, session_ptr_t session) {
			m_subjects.add(subject, session);
		}

		void TcpServer::unsub(session_ptr_t session) {
			m_subjects.remove(session);
		}

		void TcpServer::unsub(const char* sub, session_ptr_t session) {
			m_subjects.remove(sub, session);
		}

		void TcpServer::establish() {
			// make session
			auto session = std::make_shared<Session>(shared_from_this());

			// init session
			if (!session->prepare()) {
				return;
			}

			// accept next
			auto stream = reinterpret_cast<uv_stream_t*>(&session->get_stream());
			auto r = uv_accept(reinterpret_cast<uv_stream_t*>(&m_server), stream);
			ASSERT(r == 0);
			if (r) {
				LOG_UV_ERR(r);
				return;
			}

			// add to mgr
			m_sessions.add_session(session);

			// notify
			on_connect_finished(true, session);

			// post first read request
			session->post_read_req();
		}

		void TcpServer::on_sub(session_ptr_t session, const char* subject) {
			m_subjects.add(subject, session);
		}

		void TcpServer::on_unsub(session_ptr_t session, const char* subject) {
			m_subjects.remove(subject, session);
		}

		void TcpServer::on_connect_finished(bool success, session_ptr_t session) {
			if (m_handler) {
				m_handler->on_connected(success, session);
			}
		}

		void TcpServer::on_closed(session_ptr_t session) {
			if (m_handler) {
				m_handler->on_closed(session);
			}

			m_sessions.remove(session);
		}

		void TcpServer::on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) {
			if (m_handler) {
				m_handler->on_req(session, pcb, cb);
			}
			else {
				cb(NE_NoHandler, nullptr);
			}
		}

		error_no_t TcpServer::on_push(session_ptr_t session, cbuf_ptr_t pcb) {
			if (m_handler) {
				return m_handler->on_push(session, pcb);
			}

			return NE_NoHandler;
		}

		void TcpServer::connection_cb(uv_stream_t* server, int status) {
			if (status) {
				LOG_UV_ERR(status);
			}

			auto tcp_server = static_cast<TcpServer*>(server->data);
			tcp_server->establish();
		}
	}
}
