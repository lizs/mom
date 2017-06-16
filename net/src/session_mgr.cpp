#include "session_mgr.h"
#include "session.h"
#include "logger.h"

namespace VK {
	namespace Net {
		void SessionMgr::remove(session_ptr_t session) {
			auto it = m_sessions.find(session->get_id());
			if (it != m_sessions.end()) {
				m_sessions.erase(it);
			}
			else {
				Logger::instance().warn("Session not exist.");
			}
		}

		size_t SessionMgr::size() const {
			return m_sessions.size();
		}

		void SessionMgr::broadcast(cbuf_ptr_t pcb) {
			auto pcbs = std::move(pack(pcb, Push));
			if (pcbs.size() == 0)
				return;
			
			for (auto& kv : m_sessions) {
				auto session = kv.second;
				session->send(pcbs, nullptr);
			}
		}

		void SessionMgr::multicast(cbuf_ptr_t pcb, std::vector<session_ptr_t>& sessions) {
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

		session_ptr_t SessionMgr::get_session(int id) {
			auto it = m_sessions.find(id);
			if (it != m_sessions.end()) return it->second;
			return nullptr;
		}

		bool SessionMgr::add_session(session_ptr_t session) {
			auto it = m_sessions.find(session->get_id());
			if (it != m_sessions.end())
				return false;

			Logger::instance().debug("session {} established!", session->get_id());
			session->set_host(this);

			// retain
			m_sessions.insert(m_sessions.end(), std::make_pair(session->get_id(), session));
			return true;
		}
	}
}
