#pragma once

#include "uv_plus.h"
#include "scheduler.h"
#include "session_mgr.h"

class TcpServer
{
private:
	uv_tcp_t m_server;
	Scheduler m_scheduler;
	SessionMgr m_sessions;
	
	static void alloc_cb(uv_handle_t* handle,
		size_t suggested_size,
		uv_buf_t* buf);

	static void read_cb(uv_stream_t* handle,
		ssize_t nread,
		const uv_buf_t* buf);

	static void connection_cb(uv_stream_t * server, int status);

public:
	TcpServer()
	{
		m_server.data = this;
	}

	int start(const char * ip, int port);
	SessionMgr & get_session_mgr() { return m_sessions; }
};