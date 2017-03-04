// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include "net.h"
#include "scheduler.h"

namespace VK {

	namespace Net {
		template <typename T>
		class TcpClient final {

			const uint64_t MAX_RECONN_DELAY = 32000;
			const uint64_t DEFAULT_RECONN_DELAY = 1000;

		public:
			TYPE_DEFINES(T)

			TcpClient(const char* ip, int port,
			          open_cb_t open_cb = nullptr,
			          close_cb_t close_cb = nullptr,
			          req_handler_t req_handler = nullptr,
			          push_handler_t push_handler = nullptr,
			          bool auto_reconnect_enabled = true);
			~TcpClient();
			bool startup();
			bool shutdown();
#pragma region("Message patterns")
			void request(cbuf_t* pcb, req_cb_t rcb);
			void push(cbuf_t* pcb, send_cb_t cb);
#pragma endregion("Message patterns")

		private:
			void reconnect();
			void reconnect(uint64_t delay);
			void set_reconn_delay(uint64_t delay);
			void double_reonn_delay();

			Scheduler m_scheduler;
			T* m_session;

			bool m_autoReconnect = true;
			std::string m_ip;
			int m_port;

			uint64_t m_reconnDelay = DEFAULT_RECONN_DELAY;
		};

		template <typename T>
		TcpClient<T>::TcpClient(const char* ip, int port,
		                        typename T::open_cb_t open_cb,
		                        typename T::close_cb_t close_cb,
		                        typename T::req_handler_t req_handler,
		                        typename T::push_handler_t push_handler,
		                        bool auto_reconnect_enabled) : m_autoReconnect(auto_reconnect_enabled), m_ip(ip), m_port(port) {
			m_session = new T(
				[=, this](bool success, T* session) {
					if (!success) {
						if (m_autoReconnect) {
							reconnect();
						}
					}
					else {
						set_reconn_delay(DEFAULT_RECONN_DELAY);
						m_session->post_read_req();
					}

					if (open_cb)
						open_cb(success, session);
				},
				[=, this](T* session) {
					set_reconn_delay(DEFAULT_RECONN_DELAY);
					if (close_cb) {
						close_cb(session);

						if (m_autoReconnect) {
							reconnect();
						}
					}
				}, req_handler, push_handler);
		}

		template <typename T>
		TcpClient<T>::~TcpClient() {}

		template <typename T>
		bool TcpClient<T>::startup() {
			// session start
			if (!m_session->prepare()) {
				return false;
			}

			// connect
			if (!m_session->connect(m_ip.c_str(), m_port)) {
				return false;
			}

			return true;
		}

		template <typename T>
		void TcpClient<T>::double_reonn_delay() {
			m_reconnDelay *= 2;
			if (m_reconnDelay > MAX_RECONN_DELAY)
				m_reconnDelay = MAX_RECONN_DELAY;
		}

		template <typename T>
		void TcpClient<T>::reconnect() {
			m_session->prepare();
			double_reonn_delay();

			m_scheduler.invoke(m_reconnDelay, [this]() {
				                   m_session->connect(m_ip.c_str(), m_port);
			                   });
		}

		template <typename T>
		void TcpClient<T>::reconnect(uint64_t delay) {
			set_reconn_delay(delay);
			reconnect();
		}

		template <typename T>
		void TcpClient<T>::set_reconn_delay(uint64_t delay) {
			m_reconnDelay = delay;
		}

		template <typename T>
		bool TcpClient<T>::shutdown() {
			return m_session->close();
		}

		template <typename T>
		void TcpClient<T>::request(cbuf_t* pcb, req_cb_t cb) {
			m_session->request(pcb, cb);
		}

		template <typename T>
		void TcpClient<T>::push(cbuf_t* pcb, send_cb_t cb) {
			m_session->push(pcb, cb);
		}
	}
}
