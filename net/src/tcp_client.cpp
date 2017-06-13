#include "tcp_client.h"
#include "session.h"

namespace VK {
	namespace Net {
		TcpClient::TcpClient(const char* host, int port,
		                     open_cb_t open_cb,
		                     close_cb_t close_cb,
		                     req_handler_t req_handler,
		                     push_handler_t push_handler,
		                     bool auto_reconnect_enabled,
		                     bool connect_by_host) : m_autoReconnect(auto_reconnect_enabled),
		                                                    m_connectByHost(connect_by_host),
		                                                    m_host(host), m_port(port),
		                                                    m_open_cb(open_cb), m_close_cb(close_cb),
		                                                    m_keepAliveTimerId(INVALID_TIMER_ID) {
			m_session = std::make_shared<Session>(std::bind(&TcpClient::on_open, this, std::placeholders::_1, std::placeholders::_2),
			                        std::bind(&TcpClient::on_close, this, std::placeholders::_1), req_handler, push_handler)->shared_from_this();
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

			if (m_open_cb)
				m_open_cb(success, session);
		}

		void TcpClient::on_close(session_ptr_t session) {
			set_reconn_delay(DEFAULT_RECONN_DELAY);
			if (m_close_cb) {
				m_close_cb(session);

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
	}
}
