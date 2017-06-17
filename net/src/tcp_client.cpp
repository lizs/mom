#include "tcp_client.h"
#include "session.h"
#include "ihandler.h"

namespace VK {
	namespace Net {
		TcpClient::TcpClient(const char* host, int port,
		                     session_handler_ptr_t handler,
		                     bool auto_reconnect_enabled,
		                     bool connect_by_host) : m_autoReconnect(auto_reconnect_enabled),
		                                             m_connectByHost(connect_by_host),
		                                             m_port(port), m_handler(handler),
		                                             m_host(host),
		                                             m_keepAliveTimerId(INVALID_TIMER_ID) {
			m_session = std::make_shared<Session>(handler);
		}

		TcpClient::~TcpClient() {
			m_session = nullptr;
		}

		bool TcpClient::startup() const {
			// session start
			if (!m_session->prepare()) {
				return false;
			}

			// conn
			if(m_connectByHost) {
				m_session->connect_by_host(m_host.c_str(), m_port);
			}
			else {
				m_session->connect(m_host.c_str(), m_port);
			}
			return true;
		}

		void TcpClient::double_reonn_delay() {
			m_reconnDelay *= 2;
			if (m_reconnDelay > MAX_RECONN_DELAY)
				m_reconnDelay = MAX_RECONN_DELAY;
		}

		void TcpClient::on_open(bool success, session_ptr_t session) {
			if (!success) {
				if (m_autoReconnect) {
					reconnect();
				}
			}
			else {
				set_reconn_delay(DEFAULT_RECONN_DELAY);
				m_session->post_read_req();

				// ping
				m_keepAliveTimerId = m_scheduler.invoke(KEEP_ALIVE_INTERVAL,
				                                                           KEEP_ALIVE_INTERVAL,
				                                                           [this](any usr_data) {
					                                                           if (m_session->get_keep_alive_counter() > KEEP_ALIVE_COUNTER_DEAD_LINE)
						                                                           m_session->close();
					                                                           else if (m_session->get_elapsed_since_last_response() * 1000 > KEEP_ALIVE_INTERVAL)
						                                                           m_session->ping();
				                                                           });
			}

			if (m_handler)
				m_handler->on_connect_finished(success, session);
		}

		void TcpClient::on_close(session_ptr_t session) {
			set_reconn_delay(DEFAULT_RECONN_DELAY);
			if (m_handler) {
				m_handler->on_closed(session);

				if (m_autoReconnect) {
					reconnect();
				}

				m_scheduler.cancel(m_keepAliveTimerId);
			}
		}

		void TcpClient::reconnect() {
			m_session->prepare();
			double_reonn_delay();

			m_scheduler.invoke(m_reconnDelay, [this](any usr_data) {
				                   if (m_connectByHost) {
					                   m_session->connect_by_host(m_host.c_str(), m_port);
				                   }
				                   else {
					                   m_session->connect(m_host.c_str(), m_port);
				                   }
			                   });
		}

		void TcpClient::reconnect(uint64_t delay) {
			set_reconn_delay(delay);
			reconnect();
		}

		void TcpClient::set_reconn_delay(uint64_t delay) {
			m_reconnDelay = delay;
		}

		bool TcpClient::shutdown() const {
			return m_session->close();
		}

		void TcpClient::request(cbuf_ptr_t pcb, req_cb_t cb) const {
			m_session->request(pcb, cb);
		}

		void TcpClient::push(cbuf_ptr_t pcb, send_cb_t cb) const {
			m_session->push(pcb, cb);
		}

		void TcpClient::sub(const char* sub) const {
			m_session->sub(sub);			
		}

		void TcpClient::unsub(const char* sub) const {
			m_session->unsub(sub);
		}
	}
}
