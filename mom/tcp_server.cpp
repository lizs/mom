#include "tcp_server.h"
#include "session.h"

void TcpServer::connection_cb(uv_stream_t* server, int status)
{
	uv_stream_t* stream;
	int r;

	LOG_UV_ERR(status);

	// make session
	Session * session = new Session();
	stream = (uv_stream_t*)&session->get_stream();
	ASSERT(stream != NULL);

	r = uv_tcp_init(uv_default_loop(), (uv_tcp_t*)stream);
	ASSERT(r == 0);
	if (r) {
		delete session;
		LOG_UV_ERR(r);
		return;
	}

	TcpServer * tcp_server = (TcpServer*)server->data;
	tcp_server->get_session_mgr().add_session(session);

	/* associate server with stream */
	stream->data = session;

	r = uv_accept(server, stream);
	ASSERT(r == 0);
	if (r) {
		LOG_UV_ERR(r);
		return;
	}

	r = uv_read_start(stream, alloc_cb, read_cb);
	ASSERT(r == 0);
	if (r) {
		LOG_UV_ERR(r);
		return;
	}
}

void TcpServer::read_cb(uv_stream_t* handle,
                ssize_t nread,
                const uv_buf_t* buf)
{
	Session* session = (Session*)handle->data;

	if (nread < 0)
	{
		/* Error or EOF */
		//ASSERT(nread == UV_EOF);
		if (nread != UV_EOF) {
			LOG_UV_ERR(nread);
		}

		session->close();
		return;
	}

	if (nread == 0)
	{
		/* Everything OK, but nothing read. */
		session->close();
		return;
	}

	if(!session->dispatch(nread))
	{
		session->close();
	}
}

void TcpServer::alloc_cb(uv_handle_t* handle,
                      size_t suggested_size,
                      uv_buf_t* buf)
{
	circular_buf_t & circular_buf = ((Session*)handle->data)->get_cbuf();
	buf->base = circular_buf.body + circular_buf.tail;
	buf->len = circular_buf.cap - circular_buf.tail;
	if(buf->len <= 0)
	{
		ASSERT(buf->len > 0);
	}
}

int TcpServer::start(const char* ip, int port)
{	
	struct sockaddr_in addr;
	int r;
	uv_loop_t* loop;

	loop = uv_default_loop();
	r= uv_ip4_addr(ip, port, &addr);
	if (r)	{
		LOG_UV_ERR(r);
		return 1;
	}
	
	r = uv_tcp_init(loop, &m_server);
	if (r)	{
		LOG_UV_ERR(r);
		return 1;
	}

	r = uv_tcp_bind(&m_server, (const struct sockaddr*) &addr, 0);
	if (r)	{
		LOG_UV_ERR(r);
		return 1;
	}

	r = uv_listen((uv_stream_t*)&m_server, SOMAXCONN, connection_cb);
	if (r)	{
		LOG_UV_ERR(r);
		return 1;
	}

	LOG("Server listening on port : %d\n", port);

	// performance monitor
	m_scheduler.invoke(1000, 1000, []()
	{
		printf("%llu /s\n", Session::get_read());
		Session::set_read(0);
	});

	// main loop
	uv_run(loop, UV_RUN_DEFAULT);

	MAKE_VALGRIND_HAPPY();
	return 0;
}