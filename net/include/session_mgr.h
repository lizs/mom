// author : lizs
// 2017.2.22

#pragma once
#include <map>
#include "defines.h"
#include "scheduler.h"

namespace VK {
	namespace Net {		
		class TcpServer;	
		class Session;
		class CircularBuf;

		class NET_EXPORT SessionMgr final {
			typedef CircularBuf cbuf_t;
		public:
			explicit SessionMgr(TcpServer* host) : m_host(host) {}

			TcpServer* get_host() const {
				return m_host;
			}

			void close_all();
			session_ptr_t get_session(int id);
			bool add_session(session_ptr_t session);
			void remove(session_ptr_t session);
			size_t size() const;
			void close_expired();
		private:
#pragma warning(push)
#pragma warning(disable:4251)
			std::map<int, session_ptr_t> m_sessions;
#pragma warning(pop)
			TcpServer* m_host;
		};
	}
}
