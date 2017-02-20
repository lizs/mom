#pragma once
#include <functional>
#include "uv_plus.h"
#include "scheduler.h"

const uint64_t MAX_RECONN_DELAY = 32000;
const uint64_t DEFAULT_RECONN_DELAY = 1000;

class Session;
// tcp client
class TcpClient {
	Scheduler m_scheduler;
	Session * m_session;

	bool m_autoReconnect = true;
	bool m_established = false;
	std::string m_ip;
	int m_port;

	std::function<void(int)> m_connectedCallback;
	
	uint64_t m_reconnDelay = DEFAULT_RECONN_DELAY;

	void double_reonn_delay();
public:
	TcpClient(const char * ip, int port, bool auto_reconnect_enabled = true);
	virtual ~TcpClient();

	int connect(std::function<void(int)> cb = nullptr);
	void reconnect();
	void reconnect(uint64_t delay);
	void set_reconn_delay(uint64_t delay);

	int close();
	int write(const char* data, u_short size, std::function<void(int)> cb) const;
};
