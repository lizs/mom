// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include "defines.h"
#include "scheduler.h"
#include <chrono>

namespace VK {
	namespace Net {
		class NET_API TcpClient final {

			const uint64_t MAX_RECONN_DELAY = 32000;
			const uint64_t DEFAULT_RECONN_DELAY = 1000;

		public:
			TcpClient(const char* ip, int port,
			          open_cb_t open_cb = nullptr,
			          close_cb_t close_cb = nullptr,
			          req_handler_t req_handler = nullptr,
			          push_handler_t push_handler = nullptr,
			          bool auto_reconnect_enabled = true,
			          bool connect_by_host = true);
			~TcpClient();
			bool startup() const;
			bool shutdown() const;
#pragma region("Message patterns")
			void request(cbuf_ptr_t pcb, req_cb_t rcb) const;
			void push(cbuf_ptr_t pcb, send_cb_t cb) const;
#pragma endregion("Message patterns")

		private:
			void reconnect();
			void reconnect(uint64_t delay);
			void set_reconn_delay(uint64_t delay);
			void double_reonn_delay();

			void on_open(bool success, session_t* session);
			void on_close(session_t* session);

			Scheduler m_scheduler;
			Session* m_session;

			bool m_autoReconnect = true;
			bool m_connectByHost = true;
			std::string m_host;
			int m_port;


			open_cb_t m_open_cb;
			close_cb_t m_close_cb;
			
			timer_id_t m_keepAliveTimerId;

			uint64_t m_reconnDelay = DEFAULT_RECONN_DELAY;
		};
	}
}
