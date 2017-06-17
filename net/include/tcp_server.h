// author : lizs
// 2017.2.22

#pragma once

#include "defines.h"
#include "scheduler.h"
#include "session_mgr.h"
#include <vector>
#include "sub_mgr.h"
#include "ihandler.h"

namespace VK {
	namespace Net {

		class NET_EXPORT TcpServer final : public ISesionHandler, public std::enable_shared_from_this<ISesionHandler> {
		public:
			~TcpServer() {}

			TcpServer(const char* ip, int port, server_handler_ptr_t handler);

			bool startup();
			bool shutdown();

			// 发布
			void pub(const char* sub, cbuf_ptr_t pcb);
			// 订阅
			void sub(const char* sub, session_ptr_t session);
			// 取消该会话的所有订阅
			void unsub(session_ptr_t session);
			// 取消某个主题的订阅
			void unsub(const char* sub, session_ptr_t session);

			// 建立会话
			void establish();

		protected:
			void on_connect_finished(bool success, session_ptr_t session) override;
			void on_closed(session_ptr_t session) override;
			void on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) override;
			error_no_t on_push(session_ptr_t session, cbuf_ptr_t pcb) override;

		private:
			void on_sub(session_ptr_t session, const char* subject) override;
			void on_unsub(session_ptr_t session, const char* subject) override;
			static void connection_cb(uv_stream_t* server, int status);

			uv_tcp_t m_server;
			Scheduler m_scheduler;
			SessionMgr m_sessions;
			SubMgr m_subjects;

#pragma warning(push)
#pragma warning(disable:4251)
			server_handler_ptr_t m_handler;
			std::string m_ip;
#pragma warning(pop)
			int m_port;
		};
	}
}
