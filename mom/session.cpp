#include "session.h"
#include "session_mgr.h"
#include "mem_pool.h"

static MemoryPool<write_req_t> g_wrPool;
static MemoryPool<CircularBuf<64>> g_messagePool;
uint64_t Session::g_readed = 0;
uint32_t Session::g_id = 0;

Session::Session() : m_host(nullptr), m_id(++g_id)
{
}

Session::~Session()
{
}

//void Session::on_message(package_t* package)
//{
//	//printf("%s\n", package->body);
//	//char * response = "Awsome man!!!";
//	//write(response, strlen(response), [](int) {});
//}

void Session::set_host(SessionMgr* mgr)
{
	m_host = mgr;
}

int Session::get_id() const
{
	return m_id;
}

uv_tcp_t& Session::get_stream()
{
	return m_stream;
}

CircularBuf<>& Session::get_cbuf()
{
	return m_cbuf;
}

int Session::close()
{
	if(m_host)
	{
		m_host->remove(this);
		m_host = nullptr;
	}

	int r;
	m_sreq.data = this;
	r = uv_shutdown(&m_sreq, (uv_stream_t*)&m_stream, [](uv_shutdown_t* req, int status)
	                {
		                req->handle->data = req->data;
		                uv_close((uv_handle_t*)req->handle, [](uv_handle_t* stream)
		                         {
			                         Session* session = (Session *)stream->data;
									 LOG("Session %d closed!", session->get_id());
			                         delete session;
		                         });
	                });

	if(r)
	{
		LOG_UV_ERR(r);
		return 1;
	}

	return r;
}

int Session::write(const char* data, uint16_t size, std::function<void(int)> cb)
{
	write_req_t* wr;

	wr = g_wrPool.newElement();
	wr->cb = cb;

	auto * pcb = g_messagePool.newElement();
	pcb->reset();
	pcb->write(size);
	pcb->write_binary(const_cast<char*>(data), size);

	wr->pcb = pcb;
	wr->buf = uv_buf_init(pcb->get_head(), pcb->get_readable_len());

	int r = uv_write(&wr->req,
	                 reinterpret_cast<uv_stream_t*>(&m_stream),
	                 &wr->buf, 1,
	                 [](uv_write_t* req, int status)
	                 {
		                 write_req_t* wr;

		                 /* Free the read/write buffer and the request */
		                 wr = reinterpret_cast<write_req_t*>(req);
		                 if (wr->cb != nullptr)
		                 {
			                 wr->cb(status);
		                 }

						 g_messagePool.deleteElement(wr->pcb);
						 g_wrPool.deleteElement(wr);

		                 LOG_UV_ERR(status);
	                 });

	if (r) {
		g_messagePool.deleteElement(wr->pcb);
		g_wrPool.deleteElement(wr);
		LOG_UV_ERR(r);
		return 1;
	}

	return 0;
}

bool Session::dispatch(ssize_t nread)
{
	auto & cbuf = get_cbuf();
	cbuf.move_tail(static_cast<ushort>(nread));

	while (true)
	{
		if (pack_desired_size == 0)
		{
			// get package len
			if (cbuf.get_len() >= sizeof(ushort))
			{
				pack_desired_size = cbuf.read<ushort>();

				if (pack_desired_size > DEFAULT_CIRCULAR_BUF_SIZE)
				{
					LOG("package much too huge : %d bytes", pack_desired_size);
					return false;
				}
			}
			else
				break;
		}

		// get package body
		if (pack_desired_size > 0 && pack_desired_size <= cbuf.get_len())
		{
			// package construct
//			package_t* package = make_package(cbuf.get_head(), pack_desired_size);
//			on_message(package);
//			free_package(package);
			++g_readed;

			cbuf.move_head(pack_desired_size);
			pack_desired_size = 0;
		}
		else
			break;
	}

	// arrange cicular buffer
	cbuf.arrange();

	return true;
}