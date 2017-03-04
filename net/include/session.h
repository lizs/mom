// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include <map>
#include "net.h"
#include "circular_buf.h"
#include "mem_pool.h"
#include "singleton.h"

namespace VK {

	namespace Net {
		template <typename T>
		class SessionMgr;

		// represents a session between server and client
		template <cbuf_len_t Size = 1024>
		class Session final {
			template <typename TSession>
			friend class TcpServer;
			template <typename TSession>
			friend class TcpClient;

		public:
			SESSION_TYPE_DEFINES

			explicit Session(open_cb_t open_cb, close_cb_t close_cb, req_handler_t req_handler, push_handler_t push_handler);
			~Session();

			bool prepare();
			bool close();
			bool connect(const char* ip, int port);

			// host 为TcpClient/TcpServer
			void set_host(void* host);
			void* get_host() const;

			// 
			void notify_established(bool open);

			int get_id() const;
			uv_tcp_t& get_stream();
			cbuf_t& get_read_cbuf();

#pragma region("Message patterns")
			// req/rep pattern
			void request(cbuf_t* pcb, req_cb_t req_cb);
			// push pattern
			void push(cbuf_t* pcb, send_cb_t cb);
#pragma endregion("Message patterns")

			// post read request
			// normally requested by a TcpServer
			// TcpClient will post it when session established automatically
			bool post_read_req();

		private:
			// dispatch packages
			bool dispatch(ssize_t nread);

			// response
			void response(error_no_t en, serial_t serial, cbuf_t* pcb, send_cb_t cb);

			// handle messages
			void on_message(cbuf_t* pcb);

			// response
			void on_response(cbuf_t* pcb);

			// send data through underline stream
			template <typename ... Args>
			void send(cbuf_t* pcb, send_cb_t cb, Args ... args);
			void send(cbuf_t* pcb, send_cb_t cb);

			// 
			bool enqueue_req(req_cb_t cb);

		private:
			void* m_host;

			// request pool
			std::map<serial_t, req_cb_t> m_requestsPool;

			// cb
			open_cb_t m_openCB;
			close_cb_t m_closeCB;

			// handlers
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
				send_cb_t cb;
				cbuf_t* pcb;
			} write_req_t;

			static MemoryPool<write_req_t> g_wrPool;
			//static MemoryPool<cbuf_t> g_messagePool;

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
				                  buf->base = cbuf.get_tail_ptr();
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
		void Session<Size>::set_host(void* host) {
			m_host = host;
		}

		template <cbuf_len_t Size>
		void* Session<Size>::get_host() const {
			return m_host;
		}

		template <cbuf_len_t Size>
		void Session<Size>::notify_established(bool open) {
			if (m_openCB) {
				m_openCB(open, this);
			}
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
		void Session<Size>::send(cbuf_t* pcb, send_cb_t cb) {
			auto wr = g_wrPool.newElement();
			wr->cb = cb;
			wr->pcb = pcb;
			wr->buf = uv_buf_init(pcb->get_head_ptr(), pcb->get_len());

			int r = uv_write(&wr->req,
			                 reinterpret_cast<uv_stream_t*>(&m_stream),
			                 &wr->buf, 1,
			                 [](uv_write_t* req, int status) {
				                 auto wr = reinterpret_cast<write_req_t*>(req);

				                 // release cbuf
				                 RELEASE_CBUF(wr->pcb);

				                 if (wr->cb) {
					                 wr->cb(!status);
				                 }

#if MONITOR_ENABLED
				                 MONITOR->dec_writing();
				                 if (!status) {
					                 MONITOR->inc_wroted();
				                 }
#endif
				                 g_wrPool.deleteElement(wr);
				                 LOG_UV_ERR(status);
			                 });

			if (r) {
				g_wrPool.deleteElement(wr);

				// release cbuf
				RELEASE_CBUF(pcb);
				if (cb)
					cb(false);

				LOG_UV_ERR(r);
			}
#if MONITOR_ENABLED
			else
			MONITOR->inc_writing();
#endif
		}

		template <cbuf_len_t Size>
		template <typename ... Args>
		void Session<Size>::send(cbuf_t* pcb, send_cb_t cb, Args ... args) {
			do {
				// 1 pattern
				// 2 serial ...
				if (!pcb->write_head(args...))
					break;

				// rewrite the package size
				if (!pcb->template write_head<pack_size_t>(pcb->get_len()))
					break;

				send(pcb, cb);
				return;
			}
			while (0);

			// release cbuf
			RELEASE_CBUF(pcb);

			if (cb)
				cb(false);
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
					case Push: {
						if (m_pushHandler) {
							// 此处没有release pcb，因为pcb在handler中的行为是不确定的，handler有可能重用pcb，故将pcb的释放交给handler
							// 类似uv中的uv_buf_t
							m_pushHandler(this, pcb);
							return;
						}
						break;
					}

					case Request: {
						serial_t serial;
						if (!pcb->template read<serial_t>(serial)) {
							close();
							break;
						}

						if (m_reqHandler) {
							// 此处没有release pcb，因为pcb在handler中的行为是不确定的，handler有可能重用pcb，故将pcb的释放交给handler
							// 类似uv中的uv_buf_t
							m_reqHandler(this, pcb, [this, serial](error_no_t en, cbuf_t* pcb) {
								             response(en, serial, pcb, nullptr);
							             });
						}
						else {
							pcb->reset();
							response(NetError::NE_NoHandler, serial, pcb, nullptr);
						}

						return;
					}

					case Response:
						on_response(pcb);
						return;

					default: break;
				}
			}
			while (0);

			// release cbuf
			RELEASE_CBUF(pcb);
		}

		template <cbuf_len_t Size>
		void Session<Size>::on_response(cbuf_t* pcb) {
			do {
				serial_t serial;
				if (!pcb->template read<serial_t>(serial)) {
					LOG("read serial from response failed");
					break;
				}

				auto it = m_requestsPool.find(serial);
				if (it == m_requestsPool.end()) {
					LOG("request serial %d not found", serial);
					break;
				}

				error_no_t en;
				if (!pcb->template read<error_no_t>(en)) {
					LOG("read error no from response failed");
					pcb->reset();
					it->second(NE_ReadErrorNo, pcb);
				}
				else
					it->second(en, pcb);

				m_requestsPool.erase(it);
				return;
			}
			while (0);

			// release cbuf
			RELEASE_CBUF(pcb);
		}

		template <cbuf_len_t Size>
		bool Session<Size>::enqueue_req(req_cb_t cb) {
			auto serial = ++m_serial;
			if (m_requestsPool.find(serial) != m_requestsPool.end()) {
				LOG("serial conflict!");

				if (cb)
					cb(NetError::NE_SerialConflict, nullptr);

				return false;
			}

			m_requestsPool.insert(std::make_pair(serial, cb));
			return true;
		}

		template <cbuf_len_t Size>
		void Session<Size>::request(cbuf_t* pcb, req_cb_t req_cb) {
			if (!enqueue_req(req_cb)) {
				RELEASE_CBUF(pcb);
				return;
			}

			auto serial = m_serial;
			send(pcb, [this, serial, pcb](bool success) {
				     if (!success) {
					     auto it = m_requestsPool.find(serial);
					     if (it == m_requestsPool.end()) {
						     LOG("request serial %d not found", serial);

						     RELEASE_CBUF(pcb);
						     return;
					     }

					     pcb->reset();
					     it->second(NetError::NE_Write, pcb);
					     m_requestsPool.erase(it);
				     }
			     }, serial, static_cast<pattern_t>(Pattern::Request));
		}

		template <cbuf_len_t Size>
		void Session<Size>::push(cbuf_t* pcb, send_cb_t cb) {
			send(pcb, cb, static_cast<pattern_t>(Pattern::Push));
		}

		template <cbuf_len_t Size>
		void Session<Size>::response(error_no_t en, serial_t serial, cbuf_t* pcb, send_cb_t cb) {
			send(pcb, cb, en, serial, static_cast<pattern_t>(Pattern::Response));
		}

		template <cbuf_len_t Size>
		bool Session<Size>::close() {
			int r;

			// make every requests fail
			for (auto& kv : m_requestsPool) {
				kv.second(NE_SessionClosed, nullptr);
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

				                shutdown_req_t* sr = reinterpret_cast<shutdown_req_t*>(req);
				                LOG("Session %d closed!", sr->session->get_id());
				                if (sr->cb) {
					                sr->cb(sr->session);
				                }

				                //req->handle->data = req;
				                uv_close(reinterpret_cast<uv_handle_t*>(req->handle), [](uv_handle_t* stream) {
					                         /*                       shutdown_req_t* sr = reinterpret_cast<shutdown_req_t*>(stream->data);
					                                                LOG("Session %d closed!", sr->session->get_id());
	                       
					                                                if (sr->cb) {
						                                                sr->cb(sr->session);
					                                                }*/
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
					auto* pcb = NEW_CBUF();
					pcb->reset();
					pcb->write_binary(cbuf.get_head_ptr(), pack_desired_size);

					// handle message
					on_message(pcb);


#if MONITOR_ENABLED
					// performance monitor
					MONITOR->inc_readed();
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
}
