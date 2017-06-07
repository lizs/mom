#include "session.h"
#include "singleton.h"
#include "monitor.h"
#include <ctime>
#include <util.h>

namespace VK {
	namespace Net {

		session_id_t Session::g_sessionId = 0;

		Session::Session(open_cb_t open_cb, close_cb_t close_cb, req_handler_t req_handler, push_handler_t push_handler) :
			m_host(nullptr),
			m_openCB(open_cb), m_closeCB(close_cb),
			m_reqHandler(req_handler),
			m_pushHandler(push_handler), m_id(++g_sessionId), m_cbuf(),
			m_lastPingTime(time(nullptr)), m_lastResponseTime(time(nullptr)), m_keepAliveCounter(0) {
			m_cbuf.reset(MAX_PACKAGE_SIZE);
		}

		Session::~Session() { }

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

			m_cbuf.reuse();
			m_stream.data = this;
			r = uv_read_start(reinterpret_cast<uv_stream_t*>(&m_stream),
			                  [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
				                  auto session = static_cast<session_t*>(handle->data);
				                  auto& cbuf = session->get_read_cbuf();
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
					m_openCB(false, this);
				}
				return;
			}

			connect(reinterpret_cast<sockaddr*>(&addr));
		}

		void Session::connect_by_host(const char* host, int port) {
			get_addr_info(host, port, [this](bool success, sockaddr* addrinfo) {
				              connect(addrinfo);
			              });
		}

		void Session::set_host(void* host) {
			m_host = host;
		}

		void* Session::get_host() const {
			return m_host;
		}

		void Session::notify_established(bool open) {
			if (m_openCB) {
				m_openCB(open, this);
			}
		}

		int Session::get_id() const {
			return m_id;
		}

		uv_tcp_t& Session::get_stream() {
			return m_stream;
		}

		cbuf_t& Session::get_read_cbuf() {
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
					cb(false, this);

				return;
			}

			auto wr = wr_pool_t::instance().alloc();
			wr->cb = cb;
			for(auto i=0; i < pcbs.size(); ++i) {
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
				wr->uv_buf_array, cnt,
				[](uv_write_t* req, int status) {
				auto wr = reinterpret_cast<write_req_t*>(req);
				auto cb = wr->cb;
				auto session = wr->session;
#if MONITOR_ENABLED
				Singleton<Monitor>::instance().dec_pending();
				if (!status) {
					Singleton<Monitor>::instance().inc_wroted();
				}
#endif
				// 清理wr
				wr->clear();
				wr_pool_t::instance().dealloc(wr);

				if (cb) {
					cb(!status, session);
				}

				LOG_UV_ERR(status);
			});

			if (r) {
				wr->clear();
				wr_pool_t::instance().dealloc(wr);

				if (cb)
					cb(false, this);

				LOG_UV_ERR(r);
			}
#if MONITOR_ENABLED
			else {
				Singleton<Monitor>::instance().inc_pending();
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

			if(cnt < 1) {
				close();
				return;
			}
			
			m_pcbArray.push_back(pcb);
			if(cnt == 1) {
				// 组包
				auto totalLen = 0;
				for(auto p : m_pcbArray) {
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
					m_pushHandler(this, pcb);
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
					m_reqHandler(this, pcb, [this, serial](error_no_t en, cbuf_ptr_t pcb) {
						response(en, serial, pcb, nullptr);
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
					LOG("read serial from response failed");
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
				LOG("request serial %d not found", serial);
				return;
			}

			error_no_t en;
			if (!pcb->read<error_no_t>(en)) {
				LOG("read error no from response failed");
				pcb->reset();
				it->second(this, NE_ReadErrorNo, pcb);
			}
			else {
				it->second(this, en, pcb);
			}

			m_requestsPool.erase(it);
		}

		bool Session::enqueue_req(req_cb_t cb) {
			auto serial = ++m_serial;
			if (m_requestsPool.find(serial) != m_requestsPool.end()) {
				LOG("serial conflict!");

				if (cb)
					cb(this, NE_SerialConflict, nullptr);

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

		void Session::get_addr_info(const char* host, int port, std::function<void(bool, sockaddr*)> cb) {
			m_greq.cb = cb;

			char device[32] = {0};
			sprintf(device, "%d", port);

			auto ret = uv_getaddrinfo(uv_default_loop(), &m_greq.req,
			                          [](uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
				                          auto cb = reinterpret_cast<getaddr_req_t*>(req)->cb;

				                          if (status) {
					                          LOG_UV_ERR(status);
					                          cb(false, nullptr);
				                          }
				                          else {
					                          cb(true, res->ai_addr);
				                          }
				                          uv_freeaddrinfo(res);
			                          }, host, device, nullptr/*&hints*/);
			if (ret) {
				LOG_UV_ERR(ret);
				cb(false, nullptr);
			}
		}

		void Session::connect(sockaddr* addr) {
			int r;

			ZeroMemory(&m_creq, sizeof(connect_req_t));
			m_creq.cb = m_openCB;
			m_creq.session = this;

			r = uv_tcp_connect(&m_creq.req,
			                   &m_stream,
			                   addr,
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
				if (m_openCB != nullptr)
					m_openCB(false, this);
			}
		}

		void Session::request(cbuf_ptr_t pcb, req_cb_t req_cb) {
			if (!enqueue_req(req_cb)) {
				return;
			}

			auto serial = m_serial;
			send(pcb, [this, serial](bool success, session_t * session) {
				     if (!success) {
					     auto it = m_requestsPool.find(serial);
					     if (it == m_requestsPool.end()) {
						     LOG("request serial %d not found", serial);

						     return;
					     }

					     it->second(this, NE_Write, nullptr);
					     m_requestsPool.erase(it);
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
			int r;
			stop_read();

			// make every requests fail
			for (auto& kv : m_requestsPool) {
				kv.second(this, NE_SessionClosed, nullptr);
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

				                auto sr = reinterpret_cast<shutdown_req_t*>(req);
				                // LOG("Session %d closed!", sr->session->get_id());
				                /*			                if (sr->cb) {
								                                sr->cb(sr->session);
							                                }*/

				                req->handle->data = req;
				                uv_close(reinterpret_cast<uv_handle_t*>(req->handle), [](uv_handle_t* stream) {
					                         auto sr = reinterpret_cast<shutdown_req_t*>(stream->data);
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

		bool Session::dispatch(ssize_t nread) {
			auto& cbuf = get_read_cbuf();
			cbuf.move_tail(static_cast<pack_size_t>(nread));

			while (true) {
				if (pack_desired_size == 0) {
					// get package len
					if (cbuf.get_len() >= sizeof(pack_size_t)) {
						if (!cbuf.read<pack_size_t>(pack_desired_size))
							return false;

						if (pack_desired_size > MAX_PACKAGE_SIZE || pack_desired_size == 0) {
							LOG("package much too huge : %d bytes", pack_desired_size);
							pack_desired_size = 0;
							return false;
						}
					}
					else
						break;
				}

				// get package body
				if (pack_desired_size <= cbuf.get_len()) {
					// copy
					auto pcb = alloc_cbuf(pack_desired_size);
					pcb->write_binary(cbuf.get_head_ptr(), pack_desired_size);
					
#if MESSAGE_TRACK_ENABLED
					pcb->write_head<cbuf_len_t>(pack_desired_size);
					PRINT_MESSAGE(pcb->get_head_ptr(), pcb->get_len(), "Read : ");
					pcb->move_head(sizeof(cbuf_len_t));
#endif

					// handle message
					on_message(pcb);

#if MONITOR_ENABLED
					// performance monitor
					Singleton<Monitor>::instance().inc_readed();
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
