// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include "bull.h"
#include "circular_buf.h"
#include "mem_pool.h"

#define DEFAULT_CIRCULAR_BUF_SIZE 1024

namespace Bull {
	template <typename T>
	class SessionMgr;

	// represents a session between server and client
	template <typename T>
	class Session {
		template <typename TSession>
		friend class TcpServer;
		template <typename TSession>
		friend class TcpClient;

	public:
		typedef CircularBuf<T::CircularBufCapacity::Value> circular_buf_t;

		explicit Session(std::function<void(bool, Session<T>*)> open_cb, std::function<void(Session<T>*)> close_cb);
		~Session();

		bool prepare();
		bool close();
		bool connect(const char* ip, int port);

		void set_host(SessionMgr<Session<T>>* mgr);
		SessionMgr<Session<T>>* get_host();

		int get_id() const;
		uv_tcp_t& get_stream();
		circular_buf_t& get_read_cbuf();

#pragma region("Message patterns")
		// req/rep pattern
		bool request(const char* data, pack_size_t size, std::function<void(bool, circular_buf_t*)> cb);
		// push pattern
		bool push(const char* data, pack_size_t size, std::function<void(bool)> cb);
#pragma endregion("Message patterns")

		// post read request
		// normally requested by a TcpServer
		// TcpClient will post it when session established automatically
		bool post_read_req();

	private:
		// dispatch packages
		bool dispatch(ssize_t nread);

		// response
		//void response(bool success, circular_buf_t* pcb, std::function<void(bool)> cb);
		bool response(bool success, serial_t serial, const char* data, pack_size_t size, std::function<void(bool)> cb);

		// handle messages
		void on_message(circular_buf_t* pcb);

		// response
		void on_response(circular_buf_t* pcb);

		// write data through underline stream
		template <typename ... Args>
		bool write(const char* data, pack_size_t size, std::function<void(bool)> cb, Args ... args);
		template <typename ... Args>
		bool write(circular_buf_t* pcb, std::function<void(bool, circular_buf_t*)> cb);

	private:
		T m_handler;
		SessionMgr<Session<T>>* m_host;

		// request pool
		std::map<serial_t, std::function<void(bool, circular_buf_t*)>> m_requestsPool;

		std::function<void(bool, Session<T>*)> m_openCallback;
		std::function<void(Session<T>*)> m_closeCallback;

		// read buf
		circular_buf_t m_cbuf;
		// underline uv stream
		uv_tcp_t m_stream;

		typedef struct {
			uv_connect_t req;
			Session<T>* session;
			std::function<void(bool, Session<T>*)> cb;
		} connect_req_t;

		// connect request
		connect_req_t m_creq;

		typedef struct {
			uv_shutdown_t req;
			Session<T>* session;
			std::function<void(Session<T>*)> cb;
		} shutdown_req_t;

		// shutdown request
		shutdown_req_t m_sreq;

		// session id
		// unique in current process
		uint32_t m_id;

		typedef struct {
			uv_write_t req;
			uv_buf_t buf;
			std::function<void(bool, circular_buf_t*)> cb;
			circular_buf_t* pcb;
		} write_req_t;

		static MemoryPool<write_req_t> g_wrPool;
		static MemoryPool<circular_buf_t> g_messagePool;

		// session id seed
		static session_id_t g_sessionId;

		// serial seed
		serial_t m_serial = 0;

		// tmp len
		uint16_t pack_desired_size = 0;
	};

	template <typename T>
	session_id_t Session<T>::g_sessionId = 0;

	template <typename T>
	MemoryPool<typename Session<T>::write_req_t> Session<T>::g_wrPool;
	template <typename T>
	MemoryPool<typename Session<T>::circular_buf_t> Session<T>::g_messagePool;

	template <typename T>
	Session<T>::Session(std::function<void(bool, Session<T>*)> open_cb, std::function<void(Session<T>*)> close_cb) :
		m_host(nullptr),
		m_openCallback(open_cb), m_closeCallback(close_cb),
		m_id(++g_sessionId) { }

	template <typename T>
	Session<T>::~Session() { }

	template <typename T>
	bool Session<T>::prepare() {
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

	template <typename T>
	bool Session<T>::post_read_req() {
		int r;

		m_stream.data = this;
		r = uv_read_start(reinterpret_cast<uv_stream_t*>(&m_stream),
		                  [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
			                  auto& cbuf = static_cast<Session<T>*>(handle->data)->get_read_cbuf();
			                  buf->base = cbuf.get_tail();
			                  buf->len = cbuf.get_writable_len();
		                  },
		                  [](uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
			                  auto session = static_cast<Session<T>*>(handle->data);

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
				                  session->close();
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

	template <typename T>
	bool Session<T>::connect(const char* ip, int port) {
		struct sockaddr_in addr;
		uv_loop_t* loop;
		int r;

		loop = uv_default_loop();

		r = uv_ip4_addr(ip, port, &addr);
		if (r) {
			LOG_UV_ERR(r);
			return false;
		}

		ZeroMemory(&m_creq, sizeof(connect_req_t));
		m_creq.cb = m_openCallback;
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

	template <typename T>
	void Session<T>::set_host(SessionMgr<Session<T>>* mgr) {
		m_host = mgr;
	}

	template <typename T>
	SessionMgr<Session<T>>* Session<T>::get_host() {
		return m_host;
	}

	template <typename T>
	int Session<T>::get_id() const {
		return m_id;
	}

	template <typename T>
	uv_tcp_t& Session<T>::get_stream() {
		return m_stream;
	}

	template <typename T>
	typename Session<T>::circular_buf_t& Session<T>::get_read_cbuf() {
		return m_cbuf;
	}

	template <typename T>
	template <typename ... Args>
	bool Session<T>::write(const char* data, pack_size_t size, std::function<void(bool)> cb, Args ... args) {
		auto* pcb = g_messagePool.newElement();
		pcb->reset();

		// 1 pattern
		// 2 serial
		pcb->write(args...);

		// 3 data
		pcb->write_binary(const_cast<char*>(data), size);

		// rewrite the package size
		pcb->template write_head<pack_size_t>(pcb->get_readable_len());

		return write(pcb, [cb](bool success, circular_buf_t* pcb) {
			             g_messagePool.deleteElement(pcb);
			             if (cb)
				             cb(success);
		             });
	}

	template <typename T>
	void Session<T>::on_message(circular_buf_t* pcb) {
		auto pattern = static_cast<Pattern>(pcb->template read<pattern_t>());
		switch (pattern) {
			case Push:
				m_handler.on_push(pcb->get_head(), pcb->get_readable_len());
				break;

			case Request: {
				char* responseData;
				cbuf_len_t responseDataSize;
				auto serial = pcb->template read<serial_t>();
				auto result = m_handler.on_request(pcb->get_head(), pcb->get_readable_len(), &responseData, responseDataSize);
				response(result, serial, responseData, responseDataSize, [](bool) {});

				break;
			}

			case Response:
				on_response(pcb);
				break;

			default: break;
		}

		g_messagePool.deleteElement(pcb);
	}

	template <typename T>
	void Session<T>::on_response(circular_buf_t* pcb) {
		auto serial = pcb->template read<serial_t>();
		auto it = m_requestsPool.find(serial);
		if (it == m_requestsPool.end()) {
			LOG("request serial %d not found", serial);
			return;
		}

		it->second(true, pcb);
		m_requestsPool.erase(it);
	}

	template <typename T>
	template <typename ... Args>
	bool Session<T>::write(circular_buf_t* pcb, std::function<void(bool, circular_buf_t*)> cb) {
		auto wr = g_wrPool.newElement();
		wr->cb = cb;
		wr->pcb = pcb;
		wr->buf = uv_buf_init(pcb->get_head(), pcb->get_readable_len());

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

	template <typename T>
	bool Session<T>::request(const char* data, pack_size_t size, std::function<void(bool, circular_buf_t*)> cb) {
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

	template <typename T>
	bool Session<T>::push(const char* data, pack_size_t size, std::function<void(bool)> cb) {
		return write(data, size,
		             cb,
		             static_cast<pattern_t>(Pattern::Push));
	}


	template <typename T>
	bool Session<T>::response(bool success, serial_t serial, const char* data, pack_size_t size, std::function<void(bool)> cb) {
		return write(success ? data : nullptr, success ? size : 0,
		             [cb](bool success) {
			             if (cb)
				             cb(success);
		             },
		             static_cast<pattern_t>(Pattern::Response), serial);
	}

	template <typename T>
	bool Session<T>::close() {
		int r;

		// make every requests fail
		for (auto& kv : m_requestsPool) {
			kv.second(false, nullptr);
		}
		m_requestsPool.clear();

		m_sreq.session = this;
		m_sreq.cb = m_closeCallback;
		r = uv_shutdown(reinterpret_cast<uv_shutdown_t*>(&m_sreq), reinterpret_cast<uv_stream_t*>(&m_stream), [](uv_shutdown_t* req, int status) {
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

	template <typename T>
	bool Session<T>::dispatch(ssize_t nread) {
		auto& cbuf = get_read_cbuf();
		cbuf.move_tail(static_cast<uint16_t>(nread));

		while (true) {
			if (pack_desired_size == 0) {
				// get package len
				if (cbuf.get_len() >= sizeof(uint16_t)) {
					pack_desired_size = cbuf.template read<uint16_t>();
					if (pack_desired_size > DEFAULT_CIRCULAR_BUF_SIZE) {
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
