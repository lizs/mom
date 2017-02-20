#include "session.h"
#include "session_mgr.h"

Session * SessionMgr::get_session(int id)
{
	auto it = m_sessions.find(id);
	if (it != m_sessions.end()) return it->second;
	return nullptr;
}

void SessionMgr::close_all()
{
	for(auto kv : m_sessions)
	{
		kv.second->close();
	}
}

bool SessionMgr::add_session(Session* session)
{
	auto it = m_sessions.find(session->get_id());
	if (it != m_sessions.end())
		return false;

	session->set_host(this);
	m_sessions.insert(m_sessions.end(), std::make_pair(session->get_id(), session));
	return true;
}

void SessionMgr::remove(Session* session)
{
	// vs algrithm compile bug?
	// C2166

	//auto id = session->get_id();
	//std::remove_if(m_sessions.begin(), m_sessions.end(), [=](auto kv)->bool
	//{
	//	return id == kv->first;
	//});

	auto it = m_sessions.find(session->get_id());
	ASSERT(it != m_sessions.end());
	if(it != m_sessions.end())
	{
		m_sessions.erase(it);
	}
}