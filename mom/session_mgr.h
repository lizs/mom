#pragma once
#include <map>

class Session;
class SessionMgr
{
	friend class Session;
	std::map<int, Session*> m_sessions;

	void remove(Session * session);

public:
	void close_all();
	Session * get_session(int id);
	bool add_session(Session * session);
};