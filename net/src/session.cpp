#include <ctime>
#include <memory>
#include "session.h"
#include "monitor.h"
#include "mem_pool.h"
#include "ihandler.h"

namespace VK {
	namespace Net {

		session_id_t Session::g_sessionId = 0;

		Session::Session(session_handler_ptr_t handler) :
			m_id(++g_sessionId),
			m_lastResponseTime(time(nullptr)),
			m_keepAliveCounter(0),
			m_cbuf(),
			m_handler(handler) {
			m_cbuf = alloc_cbuf(MAX_SINGLE_PACKAGE_SIZE);
		}

		Session::~Session() {
			m_cbuf = nullptr;
			m_handler = nullptr;
			m_requestsPool.clear();
			m_pcbArray.clear();
		}

		bool Session::prepare() {
			uv_loop_t* loop;
			int r;

			loop = uv_default_loop();

			r = uv_tcp_init(loop, &m_readReq.stream);
			if (r) {
				LOG_UV_ERR(r);
				return false;
			}

			r = uv_tcp_nodelay(&m_readReq.stream, 1);
			if (r) {
				LOG_UV_ERR(r);
				return false;
			}

			return true;
		}

		bool Session::post_read_req() {
			int r;

			m_cbuf->reuse();
			m_readReq.session = shared_from_this();
			r = uv_read_start(reinterpret_cast<uv_stream_t*>(&m_readReq.stream),
			                  [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
				                  auto session = reinterpret_cast<read_req_t*>(handle)->session.lock();
				                  auto cbuf = session->get_read_cbuf();
				                  buf->base = cbuf->get_tail_ptr();
				                  buf->len = cbuf->get_writable_len();
			                  },
			                  [](uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
								  auto session = reinterpret_cast<read_req_t*>(handle)->session.lock();

				                  // see : http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_cb
				                  if (nread < 0) {
					                  session->close();
					                  LOG_UV_ERR((int)nread);
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
			send(pcb, nullptr, ++m_pp_serial, static_cast<pattern_t>(Ping));
			++m_keepAliveCounter;
		}

		void Session::pong(pp_serial_t serial) {
			auto pcb = alloc_cbuf(0);
			send(pcb, nullptr, serial, static_cast<pattern_t>(Pong));
		}

		void Session::sub(const char* subject) {
			auto len = strlen(subject);
			auto pcb = alloc_cbuf(len + 1);
			if (pcb->write(subject)) {
				send(pcb, nullptr, static_cast<pattern_t>(Sub));
			}
		}

		void Session::unsub(const char* subject) {
			auto len = strlen(subject);
			auto pcb = alloc_cbuf(len + 1);
			if (pcb->write(subject)) {
				send(pcb, nullptr, static_cast<pattern_t>(Unsub));
			}
		}

		time_t Session::get_elapsed_since_last_response() const {
			return time(nullptr) - m_lastResponseTime;
		}

		uint8_t Session::get_keep_alive_counter() const {
			return m_keepAliveCounter;
		}

		void Session::connect(const char* ip_or_host, int port) {
			auto _this = shared_from_this();
			get_addr_info(ip_or_host, port, [_this](bool success, sockaddr* addrinfo) {
				              if (success) {
					              _this->connect(addrinfo);
				              }
				              else {
					              if (_this->m_handler) {
						              _this->m_handler->on_connected(false, _this);
					              }
				              }
			              });
		}

		int Session::get_id() const {
			return m_id;
		}

		uv_tcp_t& Session::get_stream() {
			return m_readReq.stream;
		}

		cbuf_ptr_t Session::get_read_cbuf() const {
			return m_cbuf;
		}

		session_handler_ptr_t Session::get_handler() const {
			return m_handler;
		}

		void Session::send(cbuf_ptr_t pcb, send_cb_t cb) {
			std::vector<cbuf_ptr_t> pcbs = {pcb};
			send(pcbs, cb);
		}

		void Session::send(std::vector<cbuf_ptr_t>& pcbs, send_cb_t cb) {
			if (!uv_is_writable(reinterpret_cast<uv_stream_t*>(&m_readReq.stream))) {
				if (cb)
					cb(false, shared_from_this());

				return;
			}

			auto cnt = pcbs.size();
			if (cnt == 0 || cnt > MAX_SLICE_COUNT) {
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
			                 reinterpret_cast<uv_stream_t*>(&m_readReq.stream),
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

			byte_t cnt = 1;
			if (!static_cast<byte_t>(pcb->read<byte_t>(cnt))) {
				close();
				return;
			}

			if (cnt < 1) {
				close();
				return;
			}

			if (cnt > MAX_SLICE_COUNT) {
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
				if (message == nullptr) {
					LOG_WARN("alloc_cbuf {} failed, so close the session.", totalLen);
					close();
					return;
				}

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
					pp_serial_t serial;
					if (!pcb->read<pp_serial_t>(serial)) {
						close();
						return;
					}

					pong(serial);
					break;
				}

				case Pong: {
					pp_serial_t serial;
					if (!pcb->read<pp_serial_t>(serial)) {
						close();
						return;
					}

					if (m_keepAliveCounter > 0)
						--m_keepAliveCounter;

					break;
				}

				case Push: {
					if (m_handler) {
						m_handler->on_push(shared_from_this(), pcb);
					}
					else
						Logger::instance().warn("No handler, push ignored");

					break;
				}

				case Request: {
					serial_t serial;
					if (!pcb->read<serial_t>(serial)) {
						close();
						return;
					}

					if (m_handler) {
						auto _this = shared_from_this();
						m_handler->on_req(shared_from_this(), pcb, [_this, serial](error_no_t en, cbuf_ptr_t pcb) {
							                  _this->response(en, serial, pcb, nullptr);
						                  });
					}
					else {
						pcb->reset();
						response(NE_NoHandler, serial, pcb, nullptr);
					}

					return;
				}

				case Response: {
					serial_t serial;
					if (!pcb->read<serial_t>(serial)) {
						Logger::instance().error("read serial from response failed");
						break;
					}

					on_response(serial, pcb);
					break;
				}

				case Sub: {
					if (m_handler) {
						m_handler->on_sub(shared_from_this(), pcb->get_head_ptr());
					}
					break;
				}

				case Unsub: {
					if (m_handler) {
						m_handler->on_unsub(shared_from_this(), pcb->get_head_ptr());
					}
					break;
				}

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

			r = uv_read_stop(reinterpret_cast<uv_stream_t*>(&m_readReq.stream));
			if (r) {
				LOG_UV_ERR(r);
			}
		}

		void Session::get_addr_info(const char* host, int port, std::function<void(bool, sockaddr*)> cb) const {
			auto greq = alloc_getaddr_req();
			greq->cb = cb;

			char service[32] = {0};
			sprintf_s(service, "%d", port);

			auto ret = uv_getaddrinfo(uv_default_loop(), &greq->req,
			                          [](uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
				                          auto greq = reinterpret_cast<getaddr_req_t*>(req);
				                          auto cb = greq->cb;

				                          if (status) {
					                          LOG_UV_ERR(status);
					                          cb(false, nullptr);
				                          }
				                          else if (res == nullptr) {
					                          cb(false, nullptr);
				                          }
				                          else {
					                          cb(true, res->ai_addr);
				                          }

				                          uv_freeaddrinfo(res);
				                          release_getaddr_req(greq);
			                          }, host, service, nullptr/*&hints*/);
			if (ret) {
				release_getaddr_req(greq);
				LOG_UV_ERR(ret);
			}
		}

		std::string Session::get_peer_ip() const {
			struct sockaddr peerAddr;
			int namelen;
			int ret;

			namelen = sizeof peerAddr;
			ret = uv_tcp_getpeername(&m_readReq.stream, &peerAddr, &namelen);
			if (ret) {
				LOG_UV_ERR(ret);
				return "";
			}

			auto* addrIn = reinterpret_cast<sockaddr_in*>(&peerAddr);
			char dst[64] = {0};
			ret = uv_inet_ntop(peerAddr.sa_family, &addrIn->sin_addr, dst, sizeof(dst) - 1);
			if (ret) {
				LOG_UV_ERR(ret);
				return "";
			}

			return std::move(std::string(dst));
		}

		void Session::connect(sockaddr* addr) {
			int r;

			auto creq = alloc_connect_req();
			//creq->cb = m_openCB;
			creq->session = shared_from_this();

			r = uv_tcp_connect(&creq->req,
			                   &m_readReq.stream,
			                   addr,
			                   [](uv_connect_t* ct, int status) {
				                   auto creq = reinterpret_cast<connect_req_t*>(ct);
				                   if (status) {
					                   LOG_UV_ERR(status);
				                   }
				                   else {
					                   Logger::instance().debug("Session {} established!", creq->session.lock()->get_id());
				                   }

				                   auto session = creq->session.lock();
				                   if (session) {
					                   session->m_handler->on_connected(!status, session);
				                   }

				                   release_connect_req(creq);
			                   });

			if (r) {
				LOG_UV_ERR(r);
				release_connect_req(creq);

				if (m_handler)
					m_handler->on_connected(false, shared_from_this());
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
			// typedef void(*uv_close_cb)(uv_handle_t* handle);
			uv_close(reinterpret_cast<uv_handle_t*>(&m_readReq.stream), [](uv_handle_t* handle) {
				         auto req = reinterpret_cast<read_req_t*>(handle);
				         auto session = req->session.lock();
				         if (session != nullptr) {
					         Logger::instance().debug("Session {} closed!", session->get_id());

					         auto handler = session->get_handler();
					         if (handler) {
						         handler->on_closed(session);
					         }
				         }
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

						if (pack_desired_size > MAX_SINGLE_PACKAGE_SIZE || pack_desired_size == 0) {
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
