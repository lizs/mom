// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include <chrono>
#include "defines.h"
#include "scheduler.h"
#include "ihandler.h"

namespace VK {
	namespace Net {

		class NET_EXPORT TcpClient final : public ISessionHandler, public std::enable_shared_from_this<ISessionHandler> {
			const uint64_t MAX_RECONN_DELAY = 32000;
			const uint64_t DEFAULT_RECONN_DELAY = 1000;

		public:
			TcpClient(const char* ip, int port,
			          handler_ptr_t handle = nullptr,
			          bool auto_reconnect_enabled = true,
			          bool connect_by_host = true);
			~TcpClient();
			bool startup();
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

			void on_req(session_ptr_t, cbuf_ptr_t, resp_cb_t) override;
			error_no_t on_push(session_ptr_t, cbuf_ptr_t) override;
			void on_connected(bool, session_ptr_t) override;
			void on_closed(session_ptr_t) override;
			void on_sub(session_ptr_t, const std::string&) override;
			void on_unsub(session_ptr_t, const std::string&) override;

		private:
			Scheduler m_scheduler;

			bool m_autoReconnect = true;
			bool m_connectByHost = true;
			int m_port;

#pragma warning(push)
#pragma warning(disable:4251)
			session_ptr_t m_session;
			handler_ptr_t m_handler;
			std::string m_host;
#pragma warning(pop)
			
			timer_id_t m_keepAliveTimerId;
			uint64_t m_reconnDelay = DEFAULT_RECONN_DELAY;
		};
	}
}
