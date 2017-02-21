#pragma once
#include <functional>
#include "uv_plus.h"
#include "circular_buf.h"

#define DEFAULT_CIRCULAR_BUF_SIZE 1024

#pragma pack(1)
typedef struct {
	uv_write_t req;
	uv_buf_t buf;
	std::function<void(int)> cb;
	CircularBuf<64>* pcb;
} write_req_t;
#pragma pack()

class SessionMgr;
// represents a session between server and client
class Session {
	friend class TcpServer;
	friend class TcpClient;

	SessionMgr * m_host;
	CircularBuf<> m_cbuf;
	uv_tcp_t m_stream;
	uv_shutdown_t m_sreq;

	// session id
	// unique in current process
	uint32_t m_id;

#if MONITOR_ENABLED
	// record packages readed since last reset
	// shared between sessions
	static uint64_t g_readed;
	// record packages wroted since last reset
	// shared between sessions
	static uint64_t g_wroted;
#endif

	// session id seed
	static uint32_t g_id;

	// tmp len
	ushort pack_desired_size = 0;

	// dispatch packages
	bool Session::dispatch(ssize_t nread);

public:
	explicit Session();
	virtual ~Session();

	void set_host(SessionMgr* mgr);
	int get_id() const;
	uv_tcp_t& get_stream();
	CircularBuf<>& get_cbuf();
	int close();

	// write data through underline stream
	int write(const char* data, u_short size, std::function<void(int)> cb);

#if MONITOR_ENABLED
	// performance monitor
	static const uint64_t & get_readed() { return g_readed; }
	static void set_readed(uint64_t cnt) { g_readed = cnt; }
	static const uint64_t & get_wroted() { return g_wroted; }
	static void set_wroted(uint64_t cnt) { g_wroted = cnt; }
#endif
};
