#include "tcp_client.h"
#include "session.h"

struct connect_req {
	uv_connect_t req;
	std::function<void(int)> cb;
	bool* established_flag;

	connect_req(bool * flag,
		std::function<void(int)> cb) : established_flag(flag),
		cb(cb) {
	}
};

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
	WRITE_CALLBACK cb;
} write_req_t;

int TcpClient::start(const char * ip, int port, std::function<void(int)> cb)
{
	struct sockaddr_in addr;
	uv_loop_t* loop;
	int r;

	loop = uv_default_loop();
	
	r = uv_ip4_addr(ip, port, &addr);
	if (r) {
		fprintf(stderr, "Get ipv4 addr error : %s\n", uv_strerror(r));
		return 1;
	}

	r = uv_tcp_init(loop, &m_client);
	if (r) {
		fprintf(stderr, "Tcp init error : %s\n", uv_strerror(r));
		return 1;
	}

	connect_req * cr = new connect_req(&m_established, cb);
	cr->req.data = cr;

	r = uv_tcp_connect(&cr->req,
		&m_client,
		(const struct sockaddr*) &addr,
		[](uv_connect_t * ct, int status) {
		connect_req * cr = (connect_req*)ct->data;
		if (status) {
			fprintf(stderr, "Connect failed\n");
			if (cr->cb)
				cr->cb(status);

			return;
		}

		*cr->established_flag = true;
		if (cr->cb)
			cr->cb(status);

		fprintf(stderr, "Established!\n");
	});

	if (r) {
		fprintf(stderr, "Tcp connect error : %s\n", uv_strerror(r));
		return 1;
	}

	r = uv_run(loop, UV_RUN_DEFAULT);
	if (r) {
		fprintf(stderr, "Run uv default loop error : %s\n", uv_strerror(r));
		return 1;
	}

	MAKE_VALGRIND_HAPPY();
	return 0;
}

int TcpClient::shutdown()
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
		});
	});

	if (r) {
		fprintf(stderr, "Tcp shutdown error\n");
		return 1;
	}

	return r;
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

	if (status) {
		fprintf(stderr,
			"uv_write error: %s - %s\n",
			uv_err_name(status),
			uv_strerror(status));
	}
}

int TcpClient::write(const char * data, u_short size, WRITE_CALLBACK cb)
{
	write_req_t* wr;
	if (!m_established) {
		fprintf(stderr, "Tcp write error, session not established\n");
		return 1;
	}

	wr = (write_req_t*)malloc(sizeof(*wr));
	ZeroMemory(wr, sizeof(write_req_t));
	wr->cb = cb;

	char * mixed = (char*)malloc(size + sizeof(u_short));
	memcpy(mixed, (char*)(&size), sizeof(u_short));
	memcpy(mixed + sizeof(u_short), data, size);
	wr->buf = uv_buf_init((char*)mixed, size + sizeof(u_short));
	
	int r = uv_write(&wr->req,
		(uv_stream_t*)&m_client,
		&wr->buf, 1,
		write_cb);

	if (r) {
		fprintf(stderr, "Tcp write error\n");
		return 1;
	}

	return 0;
}
