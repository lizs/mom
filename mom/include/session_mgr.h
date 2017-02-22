// author : lizs
// 2017.2.22

#pragma once
#include <map>

namespace Bull {
	template <typename T>
	class SessionMgr {
	public:
		void close_all();
		T* get_session(int id);
		bool add_session(T* session);
		void remove(T* session);

	private:
		std::map<int, T*> m_sessions;
	};

	template <typename T>
	void SessionMgr<T>::remove(T* session) {
		// vs algrithm compile bug?
		// C2166
		auto it = m_sessions.find(session->get_id());
		ASSERT(it != m_sessions.end());
		if (it != m_sessions.end()) {
			m_sessions.erase(it);
		}
	}

	template <typename T>
	void SessionMgr<T>::close_all() {
		for (auto kv : m_sessions) {
			kv.second->close();
		}
	}

	template <typename T>
	T* SessionMgr<T>::get_session(int id) {
		auto it = m_sessions.find(id);
		if (it != m_sessions.end()) return it->second;
		return nullptr;
	}

	template <typename T>
	bool SessionMgr<T>::add_session(T* session) {
		auto it = m_sessions.find(session->get_id());
		if (it != m_sessions.end())
			return false;

		LOG("session %d established!", session->get_id());
		session->set_host(this);
		m_sessions.insert(m_sessions.end(), std::make_pair(session->get_id(), session));
		return true;
	}
}