#include "tcp_server.h"
#include "session.h"
#include <thread>
#include <vector>
#include "logger.h"

namespace VK {
	namespace Net {

		TcpServer::TcpServer(const char* ip, int port,
		                     open_cb_t open_cb,
		                     close_cb_t close_cb,
		                     req_handler_t req_handler,
		                     push_handler_t push_handler) :
			m_sessions(this),
			m_reqHandler(req_handler), m_pushHandler(push_handler),
			m_openCB(open_cb), m_closeCB(close_cb), m_ip(ip), m_port(port) {
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

		SessionMgr& TcpServer::get_session_mgr() {
			return m_sessions;
		}

		void TcpServer::broadcast(cbuf_ptr_t pcb) {
			m_sessions.broadcast(pcb);
		}

		void TcpServer::multicast(cbuf_ptr_t pcb, std::vector<session_ptr_t>& sessions) const {
			m_sessions.multicast(pcb, sessions);
		}

		req_handler_t TcpServer::get_req_handler() const {
			return m_reqHandler;
		}

		push_handler_t TcpServer::get_push_handler() const {
			return m_pushHandler;
		}

		open_cb_t TcpServer::get_open_cb() const {
			return m_openCB;
		}

		close_cb_t TcpServer::get_close_cb() const {
			return m_closeCB;
		}

		void TcpServer::connection_cb(uv_stream_t* server, int status) {
			uv_stream_t* stream;
			int r;

			if (status) {
				LOG_UV_ERR(status);
			}

			auto tcp_server = static_cast<TcpServer*>(server->data);

			// make session
			auto open_cb = tcp_server->get_open_cb();
			auto close_cb = tcp_server->get_close_cb();
			auto session = std::make_shared<Session>(nullptr, [close_cb, tcp_server](session_ptr_t session) {
				                                         if (close_cb) {
					                                         close_cb(session);
				                                         }

				                                         tcp_server->get_session_mgr().remove(session);
			                                         },

			                                         tcp_server->get_req_handler(),
			                                         tcp_server->get_push_handler());

			session->set_host(tcp_server);

			// init session
			if (!session->prepare()) {
				return;
			}

			// accept next
			stream = reinterpret_cast<uv_stream_t*>(&session->get_stream());
			r = uv_accept(server, stream);
			ASSERT(r == 0);
			if (r) {
				LOG_UV_ERR(r);
				return;
			}

			// add to mgr
			tcp_server->get_session_mgr().add_session(session);

			// notify
			if (open_cb) {
				open_cb(true, session);
			}

			// post first read request
			session->post_read_req();
		}
	}
}
