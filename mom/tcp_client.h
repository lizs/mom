#pragma once

#include <string>
#include <functional>
#include "uv_plus.h"
#include "circular_buf.h"

typedef void(*WRITE_CALLBACK)(int);
// tcp client
class TcpClient {
private:
	bool m_autoReconnect = true;
	bool m_established = false;
	u_short m_writingPending = 0;
	
	uv_tcp_t m_client;
	uv_shutdown_t m_shutdownReq;

public:
	int start(const char * ip, int port, std::function<void(int)> cb);
	int shutdown();
	uv_tcp_t & get_stream() {
		return m_client;
	}

	int write(const char * data, u_short size, WRITE_CALLBACK cb);
};
