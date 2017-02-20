#include "session.h"
#include "session_mgr.h"

uint64_t Session::g_read = 0;
uint32_t Session::g_id = 0;

Session::Session(uint16_t capacity) : m_host(nullptr), m_id(++g_id)
{
	init_circular_buffer(capacity, &m_cbuf);
}

Session::~Session()
{
	free(m_cbuf.body);
}

void Session::on_message(package_t* package)
{
	//printf("%s\n", package->body);
	free_package(package);
	++g_read;
}

int Session::close()
{
	m_host->remove(this);

	int r;
	m_sreq.data = this;
	r = uv_shutdown(&m_sreq, (uv_stream_t*)&m_stream, [](uv_shutdown_t* req, int status)
	                {
		                req->handle->data = req->data;
		                uv_close((uv_handle_t*)req->handle, [](uv_handle_t* stream)
		                         {
			                         Session* session = (Session *)stream->data;
			                         delete session;
		                         });
	                });

	ASSERT(0 == r);
	return r;
}


int Session::write(const char* data, uint16_t size, std::function<void(int)> cb)
{
	write_req_t* wr;

	wr = new write_req_t();
	ZeroMemory(wr, sizeof(write_req_t));
	wr->cb = cb;

	char * mixed = (char*)malloc(size + sizeof(uint16_t));
	memcpy(mixed, (char*)(&size), sizeof(uint16_t));
	memcpy(mixed + sizeof(uint16_t), data, size);
	wr->buf = uv_buf_init(mixed, size + sizeof(uint16_t));

	int r = uv_write(&wr->req,
	                 (uv_stream_t*)&m_stream,
	                 &wr->buf, 1,
	                 [](uv_write_t* req, int status)
	                 {
		                 write_req_t* wr;

		                 /* Free the read/write buffer and the request */
		                 wr = (write_req_t*)req;
		                 if (wr->cb != nullptr)
		                 {
			                 wr->cb(status);
		                 }

		                 free(wr->buf.base);
		                 free(wr);

		                 LOG_UV_ERR(status);
	                 });

	if (r) {
		delete wr;
		LOG_UV_ERR(r);
		return 1;
	}

	return 0;
}

bool Session::dispatch(ssize_t nread)
{
	circular_buf_t & cbuf = get_cbuf();
	cbuf.tail += (uint16_t)nread;

	while (true)
	{
		if (cbuf.pack_desired_size == 0)
		{
			// get package len
			if (get_cbuf_len(&cbuf) >= 2)
			{
				char * data = get_cbuf_head(&cbuf);
				cbuf.pack_desired_size = ((uint16_t)data[1] << 8) + data[0];

				if (cbuf.pack_desired_size > DEFAULT_CIRCULAR_BUF_SIZE)
				{
					LOG("package much too huge : %d bytes\n", cbuf.pack_desired_size);
					return false;
				}

				cbuf.head += 2;
			}
			else
				break;
		}

		// get package body
		if (cbuf.pack_desired_size > 0 && cbuf.pack_desired_size <= get_cbuf_len(&cbuf))
		{
			// package construct
			package_t* package = make_package(get_cbuf_head(&cbuf), cbuf.pack_desired_size);
			on_message(package);

			cbuf.head += cbuf.pack_desired_size;
			cbuf.pack_desired_size = 0;
		}
		else
			break;
	}

	// arrange cicular buffer
	arrange_cbuf(&cbuf);

	return true;
}