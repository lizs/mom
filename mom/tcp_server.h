#pragma once

#include "uv_plus.h"
#include "circular_buf.h"

class Session;
class TcpServer
{
private:
	uv_tcp_t m_server;

private:
	static void alloc_cb(uv_handle_t* handle,
		size_t suggested_size,
		uv_buf_t* buf);

	static void read_cb(uv_stream_t* handle,
		ssize_t nread,
		const uv_buf_t* buf);

	static bool dispatch_packages(Session * session, ssize_t nread);
	static void connection_cb(uv_stream_t * server, int status);

public:
	int start(const char * ip, int port);
};