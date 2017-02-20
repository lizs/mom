#include "tcp_server.h"
#include "session.h"

void TcpServer::connection_cb(uv_stream_t* server, int status)
{
	uv_stream_t* stream;
	int r;

	if (status) {
		fprintf(stderr, "Connect error %s\n", uv_strerror(status));
	}

	// make session
	Session * session = new Session();
	stream = (uv_stream_t*)&session->get_stream();
	ASSERT(stream != NULL);

	r = uv_tcp_init(uv_default_loop(), (uv_tcp_t*)stream);
	ASSERT(r == 0);

	/* associate server with stream */
	stream->data = session;

	r = uv_accept(server, stream);
	ASSERT(r == 0);

	r = uv_read_start(stream, alloc_cb, read_cb);
	ASSERT(r == 0);
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
			fprintf(stderr, "Tcp read error : %s\n", uv_strerror(nread));
		}

		session->shutdown();
		return;
	}

	if (nread == 0)
	{
		/* Everything OK, but nothing read. */
		session->shutdown();
		return;
	}

	if (!dispatch_packages(session, nread))
	{
		session->shutdown();
	}
}

bool TcpServer::dispatch_packages(Session * session, ssize_t nread)
{
	circular_buf_t & cbuf = session->get_cbuf();
	cbuf.tail += (u_short)nread;

	while (true)
	{
		if (cbuf.pack_desired_size == 0)
		{
			// get package len
			if (get_cbuf_len(&cbuf) >= 2)
			{
				char * data = get_cbuf_head(&cbuf);
				cbuf.pack_desired_size = ((u_short)data[1] << 8) + data[0];

				if (cbuf.pack_desired_size > 1024)
				{
					fprintf(stderr, "package much too huge : %d bytes\n", cbuf.pack_desired_size);
					return false;
				}

				cbuf.head += 2;
			}
			else
				break;
		}

		// get package body
		if (cbuf.pack_desired_size <= get_cbuf_len(&cbuf))
		{
			// package construct
			package_t* package = make_package(get_cbuf_head(&cbuf), cbuf.pack_desired_size);
			session->on_message(package);

			cbuf.head += cbuf.pack_desired_size;
			cbuf.pack_desired_size = 0;
		}
	}

	// arrange cicular buffer
	arrange_cbuf(&cbuf);

	return true;
}

void TcpServer::alloc_cb(uv_handle_t* handle,
                      size_t suggested_size,
                      uv_buf_t* buf)
{
	circular_buf_t & circular_buf = ((Session*)handle->data)->get_cbuf();
	buf->base = circular_buf.body + circular_buf.tail;
	buf->len = circular_buf.cap - circular_buf.tail;
}

int TcpServer::start(const char* ip, int port)
{	
	struct sockaddr_in addr;
	int r;
	uv_loop_t* loop;

	loop = uv_default_loop();
	r= uv_ip4_addr(ip, port, &addr);
	if (r)	{
		/* TODO: Error codes */
		fprintf(stderr, "Get ipv4 addr error\n");
		return 1;
	}
	
	r = uv_tcp_init(loop, &m_server);
	if (r)	{
		/* TODO: Error codes */
		fprintf(stderr, "Tcp init error\n");
		return 1;
	}

	r = uv_tcp_bind(&m_server, (const struct sockaddr*) &addr, 0);
	if (r)	{
		/* TODO: Error codes */
		fprintf(stderr, "Bind error\n");
		return 1;
	}

	r = uv_listen((uv_stream_t*)&m_server, SOMAXCONN, connection_cb);
	if (r)	{
		/* TODO: Error codes */
		fprintf(stderr, "Listen error %s\n", uv_err_name(r));
		return 1;
	}

	fprintf(stderr, "Server listening on port : %d\n", port);

	// loop
	uv_run(loop, UV_RUN_DEFAULT);

	MAKE_VALGRIND_HAPPY();
	return 0;
}