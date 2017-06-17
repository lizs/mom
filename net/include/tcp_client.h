// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include <chrono>
#include "defines.h"
#include "scheduler.h"

namespace VK {
	namespace Net {

		class NET_EXPORT TcpClient final {
			const uint64_t MAX_RECONN_DELAY = 32000;
			const uint64_t DEFAULT_RECONN_DELAY = 1000;

		public:
			TcpClient(const char* ip, int port,
			          session_handler_ptr_t handle = nullptr,
			          bool auto_reconnect_enabled = true,
			          bool connect_by_host = true);
			~TcpClient();
			bool startup() const;
			bool shutdown() const;

#pragma region("Message patterns")
			void request(cbuf_ptr_t pcb, req_cb_t rcb) const;
			void push(cbuf_ptr_t pcb, send_cb_t cb) const;
			void sub(const char* sub) const;
			void unsub(const char* sub) const;
#pragma endregion("Message patterns")

		private:
			void reconnect();
			void reconnect(uint64_t delay);
			void set_reconn_delay(uint64_t delay);
			void double_reonn_delay();

			void on_open(bool success, session_ptr_t session);
			void on_close(session_ptr_t session);

			Scheduler m_scheduler;

			bool m_autoReconnect = true;
			bool m_connectByHost = true;
			int m_port;

#pragma warning(push)
#pragma warning(disable:4251)
			session_ptr_t m_session;
			session_handler_ptr_t m_handler;
			std::string m_host;
#pragma warning(pop)
			
			timer_id_t m_keepAliveTimerId;
			uint64_t m_reconnDelay = DEFAULT_RECONN_DELAY;
		};
	}
}
