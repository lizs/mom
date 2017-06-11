// author : lizs
// 2017.2.22

#pragma once
#include <map>
#include "defines.h"
#include <vector>

namespace VK {
	namespace Net {		
		class TcpServer;	
		class Session;
		class CircularBuf;

		class NET_API SessionMgr final {
			typedef CircularBuf cbuf_t;
		public:
			explicit SessionMgr(TcpServer* host) : m_host(host) {}

			TcpServer* get_host() const {
				return m_host;
			}

			void close_all();
			Session* get_session(int id);
			bool add_session(Session* session);
			void remove(Session* session);
			size_t size() const;

#pragma region ("¹ã²¥¡¢×é²¥")
			void broadcast(cbuf_ptr_t pcb);
			static void multicast(cbuf_ptr_t pcb, std::vector<Session*>& sessions);
#pragma endregion 
			
		private:
#pragma warning(push)
#pragma warning(disable:4251)
			std::map<int, Session*> m_sessions;
#pragma warning(pop)
			TcpServer* m_host;
		};
	}
}
