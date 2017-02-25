// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include "bull.h"
#include "circular_buf.h"
#include "mem_pool.h"

namespace Bull {
	template <typename T>
	class SessionMgr;

	// represents a session between server and client
	template <cbuf_len_t Size = 1024>
	class Session {
		template <typename TSession>
		friend class TcpServer;
		template <typename TSession>
		friend class TcpClient;

	public:
		typedef Session<Size> session_t;
		typedef CircularBuf<Size> cbuf_t;
		typedef std::function<void(session_t*, const char*, cbuf_len_t)> push_handler_t;
		typedef std::function<bool(session_t*, const char*, cbuf_len_t, char**, cbuf_len_t&)> req_handler_t;

		typedef std::function<void(bool, session_t*)> open_cb_t;
		typedef std::function<void(session_t*)> close_cb_t;
		typedef std::function<void(bool, cbuf_t*)> req_cb_t;
		typedef std::function<void(bool)> push_cb_t;
		typedef std::function<void(bool)> rep_cb_t;
		typedef std::function<void(bool)> write_cb_0_t;
		typedef std::function<void(bool, cbuf_t*)> write_cb_1_t;

		explicit Session(open_cb_t open_cb, close_cb_t close_cb, req_handler_t req_handler, push_handler_t push_handler);
		~Session();

		bool prepare();
		bool close();
		bool connect(const char* ip, int port);

		void set_host(SessionMgr<session_t>* mgr);
		SessionMgr<session_t>* get_host();

		int get_id() const;
		uv_tcp_t& get_stream();
		cbuf_t& get_read_cbuf();

#pragma region("Message patterns")
		// req/rep pattern
		bool request(const char* data, pack_size_t size, req_cb_t cb);
		// push pattern
		bool push(const char* data, pack_size_t size, push_cb_t cb);
#pragma endregion("Message patterns")

		// post read request
		// normally requested by a TcpServer
		// TcpClient will post it when session established automatically
		bool post_read_req();

	private:
		// dispatch packages
		bool dispatch(ssize_t nread);

		// response
		bool response(bool success, serial_t serial, const char* data, pack_size_t size, rep_cb_t cb);

		// handle messages
		void on_message(cbuf_t* pcb);

		// response
		void on_response(cbuf_t* pcb);

		// write data through underline stream
		template <typename ... Args>
		bool write(const char* data, pack_size_t size, write_cb_0_t cb, Args ... args);
		template <typename ... Args>
		bool write(cbuf_t* pcb, write_cb_1_t cb);

	private:
		SessionMgr<session_t>* m_host;

		// request pool
		std::map<serial_t, req_cb_t> m_requestsPool;

		open_cb_t m_openCB;
		close_cb_t m_closeCB;
		req_handler_t m_reqHandler;
		push_handler_t m_pushHandler;

		// read buf
		cbuf_t m_cbuf;
		// underline uv stream
		uv_tcp_t m_stream;

		typedef struct {
			uv_connect_t req;
			session_t* session;
			std::function<void(bool, session_t*)> cb;
		} connect_req_t;

		// connect request
		connect_req_t m_creq;

		typedef struct {
			uv_shutdown_t req;
			session_t* session;
			std::function<void(session_t*)> cb;
		} shutdown_req_t;

		// shutdown request
		shutdown_req_t m_sreq;

		// session id
		// unique in current process
		uint32_t m_id;

		typedef struct {
			uv_write_t req;
			uv_buf_t buf;
			std::function<void(bool, cbuf_t*)> cb;
			cbuf_t* pcb;
		} write_req_t;

		static MemoryPool<write_req_t> g_wrPool;
		static MemoryPool<cbuf_t> g_messagePool;

		// session id seed
		static session_id_t g_sessionId;

		// serial seed
		serial_t m_serial = 0;

		// tmp len
		uint16_t pack_desired_size = 0;
	};

	template <cbuf_len_t Size>
	session_id_t Session<Size>::g_sessionId = 0;

	template <cbuf_len_t Size>
	MemoryPool<typename Session<Size>::write_req_t> Session<Size>::g_wrPool;
	template <cbuf_len_t Size>
	MemoryPool<typename Session<Size>::cbuf_t> Session<Size>::g_messagePool;

	template <cbuf_len_t Size>
	Session<Size>::Session(open_cb_t open_cb, close_cb_t close_cb, req_handler_t req_handler, push_handler_t push_handler) :
		m_host(nullptr),
		m_openCB(open_cb), m_closeCB(close_cb),
		m_reqHandler(req_handler),
		m_pushHandler(push_handler), m_id(++g_sessionId) { }

	template <cbuf_len_t Size>
	Session<Size>::~Session() { }

	template <cbuf_len_t Size>
	bool Session<Size>::prepare() {
		uv_loop_t* loop;
		int r;

		loop = uv_default_loop();

		r = uv_tcp_init(loop, &m_stream);
		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		return true;
	}

	template <cbuf_len_t Size>
	bool Session<Size>::post_read_req() {
		int r;

		m_stream.data = this;
		r = uv_read_start(reinterpret_cast<uv_stream_t*>(&m_stream),
		                  [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
			                  auto& cbuf = static_cast<session_t*>(handle->data)->get_read_cbuf();
			                  buf->base = cbuf.get_tail();
			                  buf->len = cbuf.get_writable_len();
		                  },
		                  [](uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
			                  auto session = static_cast<session_t*>(handle->data);

			                  if (nread < 0) {
				                  /* Error or EOF */
				                  if (nread != UV_EOF) {
					                  LOG_UV_ERR((int)nread);
				                  }

				                  session->close();
				                  return;
			                  }

			                  if (nread == 0) {
				                  /* Everything OK, but nothing read. */
				                 // session->close();
				                  return;
			                  }

			                  if (!session->dispatch(nread)) {
				                  session->close();
			                  }
		                  });

		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		return true;
	}

	template <cbuf_len_t Size>
	bool Session<Size>::connect(const char* ip, int port) {
		struct sockaddr_in addr;
		int r;
		
		r = uv_ip4_addr(ip, port, &addr);
		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		ZeroMemory(&m_creq, sizeof(connect_req_t));
		m_creq.cb = m_openCB;
		m_creq.session = this;

		r = uv_tcp_connect(&m_creq.req,
		                   &m_stream,
		                   reinterpret_cast<const sockaddr*>(&addr),
		                   [](uv_connect_t* ct, int status) {
			                   auto cr = reinterpret_cast<connect_req_t*>(ct);
			                   if (status) {
				                   LOG_UV_ERR(status);
			                   }
			                   else {
				                   LOG("Session %d established!", cr->session->get_id());
			                   }

			                   if (cr->cb) {
				                   cr->cb(!status, cr->session);
			                   }
		                   });

		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		return true;
	}

	template <cbuf_len_t Size>
	void Session<Size>::set_host(SessionMgr<session_t>* mgr) {
		m_host = mgr;
	}

	template <cbuf_len_t Size>
	SessionMgr<Session<Size>>* Session<Size>::get_host() {
		return m_host;
	}

	template <cbuf_len_t Size>
	int Session<Size>::get_id() const {
		return m_id;
	}

	template <cbuf_len_t Size>
	uv_tcp_t& Session<Size>::get_stream() {
		return m_stream;
	}

	template <cbuf_len_t Size>
	typename Session<Size>::cbuf_t& Session<Size>::get_read_cbuf() {
		return m_cbuf;
	}

	template <cbuf_len_t Size>
	template <typename ... Args>
	bool Session<Size>::write(const char* data, pack_size_t size, write_cb_0_t cb, Args ... args) {
		auto* pcb = g_messagePool.newElement();
		do {
			pcb->reset();

			// 1 pattern
			// 2 serial
			if (!pcb->write(args...))
				break;

			// 3 data
			if (!pcb->write_binary(const_cast<char*>(data), size))
				break;

			// rewrite the package size
			if (!pcb->template write_head<pack_size_t>(pcb->get_len()))
				break;

			return write(pcb, [cb](bool success, cbuf_t* pcb) {
				             g_messagePool.deleteElement(pcb);
				             if (cb)
					             cb(success);
			             });
		}
		while (0);

		g_messagePool.deleteElement(pcb);
		return false;
	}

	template <cbuf_len_t Size>
	void Session<Size>::on_message(cbuf_t* pcb) {
		do {
			pattern_t pattern;
			if (!static_cast<Pattern>(pcb->template read<pattern_t>(pattern))) {
				close();
				break;
			}

			switch (pattern) {
				case Push:
					if (m_pushHandler)
						m_pushHandler(this, pcb->get_head(), pcb->get_len());
					break;

				case Request: {
					char* responseData;
					cbuf_len_t responseDataSize;
					serial_t serial;
					if (!pcb->template read<serial_t>(serial)) {
						close();
						break;
					}

					auto success = false;
					if (m_reqHandler)
						success = m_reqHandler(this, pcb->get_head(), pcb->get_len(), &responseData, responseDataSize);

					response(success, serial,
					         success ? responseData : nullptr,
					         success ? responseDataSize : 0, [](bool) {});

					break;
				}

				case Response:
					on_response(pcb);
					break;

				default: break;
			}
		}
		while (0);

		g_messagePool.deleteElement(pcb);
	}

	template <cbuf_len_t Size>
	void Session<Size>::on_response(cbuf_t* pcb) {
		serial_t serial;
		if (!pcb->template read<serial_t>(serial))
			return;

		auto it = m_requestsPool.find(serial);
		if (it == m_requestsPool.end()) {
			LOG("request serial %d not found", serial);
			return;
		}

		it->second(true, pcb);
		m_requestsPool.erase(it);
	}

	template <cbuf_len_t Size>
	template <typename ... Args>
	bool Session<Size>::write(cbuf_t* pcb, write_cb_1_t cb) {
		auto wr = g_wrPool.newElement();
		wr->cb = cb;
		wr->pcb = pcb;
		wr->buf = uv_buf_init(pcb->get_head(), pcb->get_len());

		int r = uv_write(&wr->req,
		                 reinterpret_cast<uv_stream_t*>(&m_stream),
		                 &wr->buf, 1,
		                 [](uv_write_t* req, int status) {
			                 auto wr = reinterpret_cast<write_req_t*>(req);
			                 if (wr->cb != nullptr) {
				                 wr->cb(!status, wr->pcb);
			                 }

#if MONITOR_ENABLED
			                 if (!status) {
				                 ++Monitor::g_wroted;
			                 }
#endif
			                 g_wrPool.deleteElement(wr);
			                 LOG_UV_ERR(status);
		                 });

		if (r) {
			g_wrPool.deleteElement(wr);
			if (cb)
				cb(false, pcb);

			LOG_UV_ERR(r);
			return false;
		}

		return true;
	}

	template <cbuf_len_t Size>
	bool Session<Size>::request(const char* data, pack_size_t size, req_cb_t cb) {
		auto serial = ++m_serial;
		if (m_requestsPool.find(serial) != m_requestsPool.end()) {
			LOG("serial conflict!");

			if (cb)
				cb(false, nullptr);

			return false;
		}

		m_requestsPool.insert(std::make_pair(serial, cb));
		return write(data, size,
		             [this, serial](bool success) {
			             if (!success) {
				             auto it = m_requestsPool.find(serial);
				             if (it == m_requestsPool.end()) {
					             LOG("request serial %d not found", serial);
					             return;
				             }

				             it->second(false, nullptr);
				             m_requestsPool.erase(it);
			             }
		             },
		             static_cast<pattern_t>(Pattern::Request), serial);
	}

	template <cbuf_len_t Size>
	bool Session<Size>::push(const char* data, pack_size_t size, push_cb_t cb) {
		return write(data, size,
		             cb,
		             static_cast<pattern_t>(Pattern::Push));
	}


	template <cbuf_len_t Size>
	bool Session<Size>::response(bool success, serial_t serial, const char* data, pack_size_t size, rep_cb_t cb) {
		return write(success ? data : nullptr, success ? size : 0,
		             [cb](bool success) {
			             if (cb)
				             cb(success);
		             },
		             static_cast<pattern_t>(Pattern::Response), serial);
	}

	template <cbuf_len_t Size>
	bool Session<Size>::close() {
		int r;

		// make every requests fail
		for (auto& kv : m_requestsPool) {
			kv.second(false, nullptr);
		}
		m_requestsPool.clear();

		m_sreq.session = this;
		m_sreq.cb = m_closeCB;
		r = uv_shutdown(reinterpret_cast<uv_shutdown_t*>(&m_sreq),
		                reinterpret_cast<uv_stream_t*>(&m_stream),
		                [](uv_shutdown_t* req, int status) {
			                if (status) {
				                LOG_UV_ERR(status);
			                }

			                req->handle->data = req;
			                uv_close(reinterpret_cast<uv_handle_t*>(req->handle), [](uv_handle_t* stream) {
				                         shutdown_req_t* sr = reinterpret_cast<shutdown_req_t*>(stream->data);
				                         LOG("Session %d closed!", sr->session->get_id());

				                         if (sr->cb) {
					                         sr->cb(sr->session);
				                         }
			                         });
		                });

		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		return true;
	}

	template <cbuf_len_t Size>
	bool Session<Size>::dispatch(ssize_t nread) {
		auto& cbuf = get_read_cbuf();
		cbuf.move_tail(static_cast<pack_size_t>(nread));

		while (true) {
			if (pack_desired_size == 0) {
				// get package len
				if (cbuf.get_len() >= sizeof(pack_size_t)) {
					if (!cbuf.template read<pack_size_t>(pack_desired_size))
						return false;

					if (pack_desired_size > Size) {
						LOG("package much too huge : %d bytes", pack_desired_size);
						return false;
					}
				}
				else
					break;
			}

			// get package body
			if (pack_desired_size > 0 && pack_desired_size <= cbuf.get_len()) {

				// copy
				auto* pcb = g_messagePool.newElement();
				pcb->reset();
				pcb->write_binary(cbuf.get_head(), pack_desired_size);

				// handle message
				on_message(pcb);

#if MONITOR_ENABLED
				// performance monitor
				++Monitor::g_readed;
#endif

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
}
