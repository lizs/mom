#pragma once
#include "uv_plus.h"
#include "circular_buf.h"
#include <functional>

#define DEFAULT_CIRCULAR_BUF_SIZE 1024

#pragma pack(1)
typedef struct {
	uv_write_t req;
	uv_buf_t buf;
	std::function<void(int)> cb;
} write_req_t;
#pragma pack()

class SessionMgr;
// represents a session between server and client
class Session {
	friend class TcpServer;
	friend class TcpClient;

	SessionMgr * m_host;
	circular_buf_t m_cbuf;
	uv_tcp_t m_stream;
	uv_shutdown_t m_sreq;
	uint32_t m_id;

	// record packages readed since last reset
	// shared between sessions
	static uint64_t g_read;
	static uint32_t g_id;

	// dispatch packages
	bool Session::dispatch(ssize_t nread);
public:
	explicit Session(uint16_t capacity = DEFAULT_CIRCULAR_BUF_SIZE);
	virtual ~Session();
	virtual void on_message(package_t* package);	

	void set_host(SessionMgr * mgr) { m_host = mgr; }

	// id
	// normally identified by underline fd
	int get_id() const { return m_id; }

	uv_tcp_t & get_stream() { return m_stream; }
	circular_buf_t & get_cbuf() { return m_cbuf; }
	int close();

	// performance monitor
	static const uint64_t & get_read() { return g_read; }
	static void set_read(uint64_t cnt) { g_read = cnt; }

	// write data through underline stream
	int write(const char* data, u_short size, std::function<void(int)> cb);
	int request(const char* data, u_short size, std::function<void(int)> cb);
};
