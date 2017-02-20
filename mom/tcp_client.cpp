#include "tcp_client.h"
#include "session.h"
#include "uv_plus.h"

#pragma pack(1)

typedef struct
{
	uv_connect_t req;
	bool* established_flag;
	TcpClient* client;
	std::function<void(int)> cb;
} connect_req_t;

#pragma pack()

TcpClient::TcpClient(const char * ip, int port, bool auto_reconnect_enabled) :
	m_ip(ip), m_port(port), m_session(nullptr),
	m_autoReconnect(auto_reconnect_enabled)
{
}

TcpClient::~TcpClient()
{
}

int TcpClient::connect(std::function<void(int)> cb)
{
	struct sockaddr_in addr;
	uv_loop_t* loop;
	int r;

	// ensure do not cover previous non-empty cb
	if(cb != nullptr)
		m_connectedCallback = cb;

	ASSERT(m_session == nullptr);
	m_session = new Session();
	loop = uv_default_loop();
	
	r = uv_ip4_addr(m_ip.c_str(), m_port, &addr);
	if (r) {
		LOG_UV_ERR(r);
		return 1;
	}

	r = uv_tcp_init(loop, &m_session->get_stream());
	if (r) {
		LOG_UV_ERR(r);
		return 1;
	}

	connect_req_t * cr = new connect_req_t();
	cr->cb = cb;
	cr->client = this;
	cr->established_flag = &m_established;

	r = uv_tcp_connect(&cr->req,
		&m_session->get_stream(),
		(const struct sockaddr*) &addr,
		[](uv_connect_t * ct, int status) {
		connect_req_t * cr = (connect_req_t*)ct;
		if (status) {
			LOG_UV_ERR(status);
			if (cr->cb)
				cr->cb(status);

			cr->client->reconnect();

			delete cr;
			return;
		}

		*cr->established_flag = true;
		if (cr->cb)
			cr->cb(status);

		delete cr;
		LOG("Established!");
	});

	if (r) {
		LOG_UV_ERR(r);
		return 1;
	}

	r = uv_run(loop, UV_RUN_DEFAULT);
	if (r) {
		LOG_UV_ERR(r);
		return 1;
	}

	MAKE_VALGRIND_HAPPY();
	return 0;
}

void TcpClient::double_reonn_delay()
{
	m_reconnDelay *= 2;
	if (m_reconnDelay > MAX_RECONN_DELAY)
		m_reconnDelay = MAX_RECONN_DELAY;
}

void TcpClient::reconnect()
{
	double_reonn_delay();

	if (m_session)
	{
		delete m_session;
		m_session = nullptr;
	}

	m_scheduler.invoke(m_reconnDelay, [this]()
	{
		connect();
	});
}

void TcpClient::reconnect(uint64_t delay)
{
	set_reconn_delay(delay);
	reconnect();
}

void TcpClient::set_reconn_delay(uint64_t delay)
{
	m_reconnDelay = delay;
}

int TcpClient::close()
{
	if (!m_established) return 0;
	if (m_session == nullptr) return 0;

	m_established = false;
	int r = m_session->close();
	m_session = nullptr;

	if(m_autoReconnect){
		set_reconn_delay(DEFAULT_RECONN_DELAY);
		reconnect();
	}

	if (r) {
		LOG_UV_ERR(r);
		return 1;
	}

	return r;
}

int TcpClient::write(const char * data, u_short size, std::function<void(int)> cb) const
{
	if(m_session == nullptr || !m_established)
	{
		LOG("Tcp write error, session not established\n");
		return 1;
	}

	return m_session->write(data, size, cb);
}
