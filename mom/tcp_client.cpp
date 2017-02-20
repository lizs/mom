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
	m_ip(ip), m_port(port),
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

	loop = uv_default_loop();
	
	r = uv_ip4_addr(m_ip.c_str(), m_port, &addr);
	if (r) {
		LOG_UV_ERR(r);
		return 1;
	}

	r = uv_tcp_init(loop, &m_client);
	if (r) {
		LOG_UV_ERR(r);
		return 1;
	}

	connect_req_t * cr = new connect_req_t();
	cr->cb = cb;
	cr->client = this;
	cr->established_flag = &m_established;

	r = uv_tcp_connect(&cr->req,
		&m_client,
		(const struct sockaddr*) &addr,
		[](uv_connect_t * ct, int status) {
		connect_req_t * cr = (connect_req_t*)ct;
		if (status) {
			LOG_UV_ERR(status);
			if (cr->cb)
				cr->cb(status);

			cr->client->reconnect(cr->cb);

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

void TcpClient::reconnect(std::function<void(int)> cb)
{
	uint64_t delay = m_retryInterval * 2;
	if (delay > MAX_RETRY_INTERVAL)
		delay = MAX_RETRY_INTERVAL;

	m_scheduler.invoke(delay, [this, cb]()
	{
		connect(cb);
	});
}

int TcpClient::close()
{
	if (!m_established) return 0;

	m_established = false;
	m_shutdownReq.data = this;

	int r = uv_shutdown(&m_shutdownReq,
		(uv_stream_t*)&m_client,
		[](uv_shutdown_t * req, int status) {

		TcpClient * client = (TcpClient*)req->data;

		ASSERT(req->handle == (uv_stream_t*)&client->get_stream());
		ASSERT(req->handle->write_queue_size == 0);

		req->handle->data = client;

		uv_close((uv_handle_t*)req->handle, [](uv_handle_t * handle) {
			TcpClient * client = (TcpClient*)handle->data;
			ASSERT(handle == (uv_handle_t*)&client->get_stream());

			if (client->auto_reconnect_enabled())
				client->reconnect(nullptr);
		});
	});

	if (r) {
		LOG_UV_ERR(r);
		return 1;
	}

	return r;
}

uv_tcp_t& TcpClient::get_stream()
{
	return m_client;
}

void write_cb(uv_write_t* req, int status) {
	write_req_t* wr;

	/* Free the read/write buffer and the request */
	wr = (write_req_t*)req;
	if (wr->cb != nullptr) {
		wr->cb(status);
	}

	free(wr->buf.base);
	free(wr);

	LOG_UV_ERR(status);
}

bool TcpClient::auto_reconnect_enabled() const
{
	return m_autoReconnect;
}

int TcpClient::write(const char * data, u_short size, std::function<void(int)> cb)
{
	write_req_t* wr;
	if (!m_established) {
		LOG("Tcp write error, session not established\n");
		return 1;
	}

	wr = new write_req_t();
	ZeroMemory(wr, sizeof(write_req_t));
	wr->cb = cb;

	char * mixed = (char*)malloc(size + sizeof(u_short));
	memcpy(mixed, (char*)(&size), sizeof(u_short));
	memcpy(mixed + sizeof(u_short), data, size);
	wr->buf = uv_buf_init(mixed, size + sizeof(u_short));
	
	int r = uv_write(&wr->req,
		(uv_stream_t*)&m_client,
		&wr->buf, 1,
		write_cb);

	if (r) {
		delete wr;
		LOG_UV_ERR(r);
		return 1;
	}

	return 0;
}
