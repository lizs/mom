// author : lizs
// 2017.2.22

#ifndef MOM_TCP_SERVER_H
#define MOM_TCP_SERVER_H

#include "defines.h"
#include "scheduler.h"
#include "session_mgr.h"
#include <vector>
#include "sub_mgr.h"
#include "ihandler.h"

namespace VK {
	namespace Net {

		class NET_EXPORT TcpServer final : public IPeerHandler, public std::enable_shared_from_this<IPeerHandler> {
		public:
			~TcpServer() {}

			TcpServer(const char* ip, int port, handler_ptr_t handler);

			bool startup();
			bool shutdown();
						
			// establish an session
			void establish();

			// get subjects manager
			SubMgr& get_sub_mgr();

		protected:
			void on_connected(bool success, session_ptr_t session) override;
			void on_closed(session_ptr_t session) override;
			void on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) override;
			error_no_t on_push(session_ptr_t session, cbuf_ptr_t pcb) override;

		private:
			void on_sub(session_ptr_t session, const std::string& subject) override;
			void on_unsub(session_ptr_t session, const std::string& subject) override;
			static void connection_cb(uv_stream_t* server, int status);

			uv_tcp_t m_server;
			Scheduler m_scheduler;
			SessionMgr m_sessions;
			SubMgr m_subjects;

#pragma warning(push)
#pragma warning(disable:4251)
			handler_ptr_t m_handler;
			std::string m_ip;
#pragma warning(pop)
			int m_port;
		};
	}
}

#endif