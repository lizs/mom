#include <ctime>
#include "util.h"
#include "session.h"
#include "_singleton_.h"
#include "monitor.h"
#include "mem_pool.h"
#include "defines.h"

namespace VK {
	namespace Net {

		session_id_t Session::g_sessionId = 0;

		Session::Session(open_cb_t open_cb, close_cb_t close_cb, req_handler_t req_handler, push_handler_t push_handler) :
			m_host(nullptr),
			m_openCB(open_cb),
			m_closeCB(close_cb),
			m_reqHandler(req_handler),
			m_pushHandler(push_handler),
			m_cbuf(),
			m_id(++g_sessionId),
			m_lastPingTime(time(nullptr)),
			m_lastResponseTime(time(nullptr)),
			m_keepAliveCounter(0) {
			m_cbuf = alloc_cbuf(MAX_PACKAGE_SIZE);
			memset(&m_stream, 0, sizeof(m_stream));
		}

		Session::~Session() {
			m_cbuf = nullptr;
			m_host = nullptr;
			m_openCB = nullptr;
			m_closeCB = nullptr;
			m_reqHandler = nullptr;
			m_requestsPool.clear();
			m_pcbArray.clear();
		}

		bool Session::prepare() {
			uv_loop_t* loop;
			int r;

			loop = uv_default_loop();

			r = uv_tcp_init(loop, &m_stream);
			if (r) {
				LOG_UV_ERR(r);
				return false;
			}

			r = uv_tcp_nodelay(&m_stream, 1);
			if (r) {
				LOG_UV_ERR(r);
				return false;
			}

			return true;
		}

		bool Session::post_read_req() {
			int r;

			m_cbuf->reuse();
			m_stream.data = this;
			r = uv_read_start(reinterpret_cast<uv_stream_t*>(&m_stream),
			                  [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
				                  auto session = static_cast<session_t*>(handle->data);
				                  auto cbuf = session->get_read_cbuf();
				                  buf->base = cbuf->get_tail_ptr();
				                  buf->len = cbuf->get_writable_len();
			                  },
			                  [](uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
				                  auto session = static_cast<session_t*>(handle->data);

				                  if (nread < 0) {
					                  /* Error or EOF */
					                  if (nread != UV_EOF) {
						                  session->close();
						                  LOG_UV_ERR((int)nread);
					                  }

					                  return;
				                  }

				                  if (nread == 0) {
					                  /* Everything OK, but nothing read. */
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

		void Session::ping() {
			auto pcb = alloc_cbuf(0);
			send(pcb, nullptr, static_cast<pattern_t>(Ping));
			++m_keepAliveCounter;
		}

		void Session::pong() {
			auto pcb = alloc_cbuf(0);
			send(pcb, nullptr, static_cast<pattern_t>(Pong));
		}

		time_t Session::get_elapsed_since_last_ping() const {
			return time(nullptr) - m_lastPingTime;
		}

		time_t Session::get_elapsed_since_last_response() const {
			return time(nullptr) - m_lastResponseTime;
		}

		uint8_t Session::get_keep_alive_counter() const {
			return m_keepAliveCounter;
		}

		void Session::connect(const char* ip, int port) {
			struct sockaddr_in addr;
			int r;

			r = uv_ip4_addr(ip, port, &addr);
			if (r) {
				LOG_UV_ERR(r);
				if (m_openCB != nullptr) {
					m_openCB(false, shared_from_this());
				}
				return;
			}

			connect(reinterpret_cast<sockaddr*>(&addr));
		}

		void Session::connect_by_host(const char* host, int port) {
			auto _this = shared_from_this();
			get_addr_info(host, port, [_this](bool success, sockaddr* addrinfo) {
				              if (success) {
					              _this->connect(addrinfo);
				              }
				              else {
					              if (_this->m_openCB != nullptr) {
						              _this->m_openCB(false, _this);
					              }
				              }
			              });
		}

		void Session::set_host(void* host) {
			m_host = host;
		}

		void* Session::get_host() const {
			return m_host;
		}

		int Session::get_id() const {
			return m_id;
		}

		uv_tcp_t& Session::get_stream() {
			return m_stream;
		}

		cbuf_ptr_t Session::get_read_cbuf() const {
			return m_cbuf;
		}

		void Session::send(cbuf_ptr_t pcb, send_cb_t cb) {
			std::vector<cbuf_ptr_t> pcbs = {pcb};
			this->send(pcbs, cb);
		}

		void Session::send(std::vector<cbuf_ptr_t>& pcbs, send_cb_t cb) {
			auto cnt = pcbs.size();
			if (cnt == 0) {
				if (cb)
					cb(false, shared_from_this());

				return;
			}

			auto wr = alloc_write_req();
			wr->cb = cb;
			for (auto i = 0; i < pcbs.size(); ++i) {
				wr->pcb_array[i] = pcbs[i];
			}

			for (auto i = 0; i < cnt; ++i) {
				wr->uv_buf_array[i] = uv_buf_init(wr->pcb_array[i]->get_head_ptr(), wr->pcb_array[i]->get_len());
			}

#if MESSAGE_TRACK_ENABLED
			for (auto pcb : pcbs) {
				PRINT_MESSAGE(pcb->get_head_ptr(), pcb->get_len(), "Write : ");
			}
#endif

			int r = uv_write(&wr->req,
			                 reinterpret_cast<uv_stream_t*>(&m_stream),
			                 wr->uv_buf_array, static_cast<unsigned int>(cnt),
			                 [](uv_write_t* req, int status) {
				                 auto wr = reinterpret_cast<write_req_t*>(req);
				                 auto cb = wr->cb;
				                 auto session = wr->session;
#if MONITOR_ENABLED
				                 monitor.dec_pending();
				                 if (!status) {
					                 monitor.inc_wroted();
				                 }
#endif

				                 release_write_req(wr);

				                 if (cb) {
					                 cb(!status, session);
				                 }

				                 LOG_UV_ERR(status);
			                 });

			if (r) {
				release_write_req(wr);

				if (cb)
					cb(false, shared_from_this());

				LOG_UV_ERR(r);
			}
#if MONITOR_ENABLED
			else {
				monitor.inc_pending();
			}
#endif
		}

		void Session::on_message(cbuf_ptr_t pcb) {
			m_lastResponseTime = time(nullptr);
			if (m_keepAliveCounter > 0)
				--m_keepAliveCounter;

			byte_t cnt = 1;
			if (!static_cast<byte_t>(pcb->read<byte_t>(cnt))) {
				close();
				return;
			}

			if (cnt < 1) {
				close();
				return;
			}

			m_pcbArray.push_back(pcb);
			if (cnt == 1) {
				// ×é°ü
				auto totalLen = 0;
				for (auto p : m_pcbArray) {
					totalLen += p->get_len();
				}

				auto message = alloc_cbuf(totalLen);
				for (auto p : m_pcbArray) {
					message->write_binary(p->get_head_ptr(), p->get_len());
				}

				dispatch_message(message);
				m_pcbArray.clear();
			}
		}

		void Session::dispatch_message(cbuf_ptr_t pcb) {
			pattern_t pattern;
			if (!static_cast<Pattern>(pcb->read<pattern_t>(pattern))) {
				close();
				return;
			}

			switch (pattern) {
				case Ping: {
					m_lastPingTime = time(nullptr);
					pong();
					break;
				}

				case Pong: {
					break;
				}

				case Push: {
					if (m_pushHandler) {
						m_pushHandler(shared_from_this(), pcb);
					}

					break;
				}

				case Request: {
					serial_t serial;
					if (!pcb->read<serial_t>(serial)) {
						close();
						return;
					}

					if (m_reqHandler) {
						auto _this = shared_from_this();
						m_reqHandler(shared_from_this(), pcb, [_this, serial](error_no_t en, cbuf_ptr_t pcb) {
							             _this->response(en, serial, pcb, nullptr);
						             });
					}
					else {
						pcb->reset();
						response(NE_NoHandler, serial, pcb, nullptr);
					}

					return;
				}

				case Response:
					serial_t serial;
					if (!pcb->read<serial_t>(serial)) {
						Logger::instance().error("read serial from response failed");
						return;
					}
					on_response(serial, pcb);
					return;

				default:
					break;
			}
		}

		void Session::on_response(serial_t serial, cbuf_ptr_t pcb) {
			auto it = m_requestsPool.find(serial);
			if (it == m_requestsPool.end()) {
				Logger::instance().error("request serial {} not found", serial);
				return;
			}

			error_no_t en;
			if (!pcb->read<error_no_t>(en)) {
				Logger::instance().error("read error no from response failed");
				pcb->reset();
				it->second(shared_from_this(), NE_ReadErrorNo, pcb);
			}
			else {
				it->second(shared_from_this(), en, pcb);
			}

			m_requestsPool.erase(it);
		}

		bool Session::enqueue_req(req_cb_t cb) {
			auto serial = ++m_serial;
			if (m_requestsPool.find(serial) != m_requestsPool.end()) {
				Logger::instance().error("serial conflict!");

				if (cb)
					cb(shared_from_this(), NE_SerialConflict, nullptr);

				return false;
			}

			m_requestsPool.insert(make_pair(serial, cb));
			return true;
		}

		void Session::stop_read() {
			int r;

			r = uv_read_stop(reinterpret_cast<uv_stream_t*>(&m_stream));
			if (r) {
				LOG_UV_ERR(r);
			}
		}

		void Session::get_addr_info(const char* host, int port, std::function<void(bool, sockaddr*)> cb) const {
			auto greq = alloc_getaddr_req();
			greq->cb = cb;

			char device[32] = {0};
			sprintf_s(device, "{%d}", port);

			auto ret = uv_getaddrinfo(uv_default_loop(), &greq->req,
			                          [](uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
				                          auto greq = reinterpret_cast<getaddr_req_t*>(req);
				                          auto cb = greq->cb;

				                          if (status) {
					                          LOG_UV_ERR(status);
					                          cb(false, nullptr);
				                          }
										  else if(res == nullptr) {
											  cb(false, nullptr);
										  }
				                          else {
					                          cb(true, res->ai_addr);
				                          }

				                          uv_freeaddrinfo(res);
				                          release_getaddr_req(greq);
			                          }, host, device, nullptr/*&hints*/);
			if (ret) {
				release_getaddr_req(greq);
				LOG_UV_ERR(ret);
			}
		}

		void Session::connect(sockaddr* addr) {
			int r;

			auto creq = alloc_connect_req();
			creq->cb = m_openCB;
			creq->session = shared_from_this();

			r = uv_tcp_connect(&creq->req,
			                   &m_stream,
			                   addr,
			                   [](uv_connect_t* ct, int status) {
				                   auto creq = reinterpret_cast<connect_req_t*>(ct);
				                   if (status) {
					                   LOG_UV_ERR(status);
				                   }
				                   else {
					                   Logger::instance().debug("Session {} established!", creq->session.lock()->get_id());
				                   }

				                   if (creq->cb) {
					                   creq->cb(!status, creq->session.lock());
				                   }

				                   release_connect_req(creq);
			                   });

			if (r) {
				LOG_UV_ERR(r);
				release_connect_req(creq);

				if (m_openCB)
					m_openCB(false, shared_from_this());
			}
		}

		void Session::request(cbuf_ptr_t pcb, req_cb_t req_cb) {
			if (!enqueue_req(req_cb)) {
				return;
			}

			auto serial = m_serial;
			auto _this = shared_from_this();
			send(pcb, [_this, serial](bool success, session_ptr_t session) {
				     if (!success) {
					     auto it = _this->m_requestsPool.find(serial);
					     if (it == _this->m_requestsPool.end()) {
						     Logger::instance().warn("request serial {} not found", serial);

						     return;
					     }

					     it->second(_this, NE_Write, nullptr);
					     _this->m_requestsPool.erase(it);
				     }
			     }, serial, static_cast<pattern_t>(Request));
		}

		void Session::push(cbuf_ptr_t pcb, send_cb_t cb) {
			send(pcb, cb, static_cast<pattern_t>(Push));
		}

		void Session::response(error_no_t en, serial_t serial, cbuf_ptr_t pcb, send_cb_t cb) {
			send(pcb, cb, en, serial, static_cast<pattern_t>(Response));
		}

		bool Session::close() {
			stop_read();

			// make every requests fail
			for (auto& kv : m_requestsPool) {
				kv.second(shared_from_this(), NE_SessionClosed, nullptr);
			}
			m_requestsPool.clear();

			// close handle
			auto creq = alloc_close_req();
			creq->cb = m_closeCB;
			creq->session = shared_from_this();
			m_stream.data = creq;
			uv_close(reinterpret_cast<uv_handle_t*>(&m_stream), [](uv_handle_t* stream) {
				         auto creq = reinterpret_cast<close_req_t*>(stream->data);
				         auto session = creq->session.lock();
				         Logger::instance().debug("Session {} closed!", session->get_id());

				         if (creq->cb) {
					         creq->cb(session);
				         }

				         release_close_req(creq);
			         });

			return true;
		}

		bool Session::dispatch(ssize_t nread) {
			auto cbuf = get_read_cbuf();
			cbuf->move_tail(static_cast<pack_size_t>(nread));

			while (true) {
				if (pack_desired_size == 0) {
					// get package len
					if (cbuf->get_len() >= sizeof(pack_size_t)) {
						if (!cbuf->read<pack_size_t>(pack_desired_size))
							return false;

						if (pack_desired_size > MAX_PACKAGE_SIZE || pack_desired_size == 0) {
							Logger::instance().error("package much too huge : {} bytes", pack_desired_size);
							pack_desired_size = 0;
							return false;
						}
					}
					else
						break;
				}

				// get package body
				if (pack_desired_size <= cbuf->get_len()) {
					// copy
					auto pcb = alloc_cbuf(pack_desired_size);
					pcb->write_binary(cbuf->get_head_ptr(), pack_desired_size);

#if MESSAGE_TRACK_ENABLED
					pcb->write_head<cbuf_len_t>(pack_desired_size);
					PRINT_MESSAGE(pcb->get_head_ptr(), pcb->get_len(), "Read : ");
					pcb->move_head(sizeof(cbuf_len_t));
#endif

					// handle message
					on_message(pcb);

#if MONITOR_ENABLED
					// performance monitor
					monitor.inc_readed();
#endif

					cbuf->move_head(pack_desired_size);
					pack_desired_size = 0;
				}
				else
					break;
			}

			// arrange cicular buffer
			cbuf->arrange();

			return true;
		}
	}
}
