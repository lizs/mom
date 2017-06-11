// author : lizs
// 2017.2.22

#pragma once

#include "defines.h"
#include "scheduler.h"
#include "session_mgr.h"
#include <vector>

namespace VK {
	namespace Net {

		class NET_API TcpServer final {
		public:
			TcpServer(const char* ip, int port,
			          open_cb_t open_cb,
			          close_cb_t close_cb,
			          req_handler_t req_handler,
			          push_handler_t push_handler);

			bool startup();
			bool shutdown();
			SessionMgr& get_session_mgr();

			// ¹ã²¥Ö§³Ö
			void broadcast(cbuf_ptr_t pcb);
			void multicast(cbuf_ptr_t pcb, std::vector<session_t*>& sessions) const;

			req_handler_t get_req_handler() const;
			push_handler_t get_push_handler() const;
			open_cb_t get_open_cb() const;
			close_cb_t get_close_cb() const;

		private:
			static void connection_cb(uv_stream_t* server, int status);

			uv_tcp_t m_server;
			Scheduler m_scheduler;
			SessionMgr m_sessions;

#pragma warning(push)
#pragma warning(disable:4251)
			req_handler_t m_reqHandler;
			push_handler_t m_pushHandler;
			open_cb_t m_openCB;
			close_cb_t m_closeCB;

			std::string m_ip;
#pragma warning(pop)
			int m_port;
		};
	}
}
