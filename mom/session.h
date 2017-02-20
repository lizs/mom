#pragma once
#include <ctime>
#include "uv_plus.h"
#include "circular_buf.h"

class Session {
private:
	circular_buf_t m_cbuf;
	uv_tcp_t m_stream;
	uv_shutdown_t m_sreq;
	const u_short DefaultCircularBufSize = 1024;

	int m_read;
	clock_t m_startTime;

public:
	Session(int capacity) : m_startTime(0) {
		init_circular_buffer(capacity, &m_cbuf);
	}

	Session() : Session(DefaultCircularBufSize) {}

	~Session() {
		free(m_cbuf.body);
	}

	virtual void on_message(package_t * package) {
		//printf("%s\n", package->body);
		free_package(package);

		if (m_read == 0)
			m_startTime = clock();

		++m_read;
		if (clock() - m_startTime > 1000){
			printf("%d /s", m_read);
			m_read = 0;
		}
	}

	uv_tcp_t & get_stream() { return m_stream; }
	circular_buf_t & get_cbuf() { return m_cbuf; }

	int shutdown() {
		int r;
		m_sreq.data = this;
		r = uv_shutdown(&m_sreq, (uv_stream_t*)&m_stream, [](uv_shutdown_t* req, int status) {
			req->handle->data = req->data;
			uv_close((uv_handle_t*)req->handle, [](uv_handle_t* stream) {
				Session * session = (Session *)stream->data;
				delete session;
			});
		});

		ASSERT(0 == r);
		return r;
	}
};
