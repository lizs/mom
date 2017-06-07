#include "session_mgr.h"
#include "session.h"
#include "circular_buf.h"

namespace VK {
	namespace Net {
		void SessionMgr::remove(Session* session) {
			// vs algrithm compile bug?
			// C2166
			auto it = m_sessions.find(session->get_id());
			ASSERT(it != m_sessions.end());
			if (it != m_sessions.end()) {
				delete it->second;
				m_sessions.erase(it);
			}
		}

		size_t SessionMgr::size() const {
			return m_sessions.size();
		}

		void SessionMgr::broadcast(cbuf_ptr_t pcb) {
			auto pcbs = std::move(pack(pcb, Push));
			if(pcbs.size() == 0)
				return;
			
			for (auto& kv : m_sessions) {
				auto session = kv.second;
				session->send(pcbs, nullptr);
			}
		}

		void SessionMgr::multicast(cbuf_ptr_t pcb, std::vector<Session*>& sessions) {
			auto pcbs = std::move(pack(pcb, Push));
			if (pcbs.size() == 0)
				return;

			for (auto session : sessions) {
				session->send(pcbs, nullptr);
			}
		}

		void SessionMgr::close_all() {
			for (auto kv : m_sessions) {
				kv.second->close();
			}
		}

		Session* SessionMgr::get_session(int id) {
			auto it = m_sessions.find(id);
			if (it != m_sessions.end()) return it->second;
			return nullptr;
		}

		bool SessionMgr::add_session(Session* session) {
			auto it = m_sessions.find(session->get_id());
			if (it != m_sessions.end())
				return false;

			LOG("session %d established!", session->get_id());
			session->set_host(this);
			m_sessions.insert(m_sessions.end(), std::make_pair(session->get_id(), session));
			return true;
		}
	}
}
