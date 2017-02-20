#pragma once
#include <functional>
#include "uv_plus.h"
#include "scheduler.h"

// tcp client
class TcpClient {
	Scheduler m_scheduler;
	bool m_autoReconnect = true;
	bool m_established = false;
	std::string m_ip;
	int m_port;
	
	const uint16_t MAX_RETRY_INTERVAL = 32000;
	uint16_t m_retryInterval = 1000;
	
	uv_tcp_t m_client;
	uv_shutdown_t m_shutdownReq;

public:
	TcpClient(const char * ip, int port, bool auto_reconnect_enabled = true);
	virtual ~TcpClient();

	uv_tcp_t& get_stream();
	bool auto_reconnect_enabled() const;

	int connect(std::function<void(int)> cb);
	void reconnect(std::function<void(int)> cb);
	int close();

	int write(const char* data, u_short size, std::function<void(int)> cb);
};
