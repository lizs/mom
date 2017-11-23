// author : lizs
// 2017.2.22

#ifndef MOM_SESSION_H
#define MOM_SESSION_H

#include <functional>
#include <map>
#include "defines.h"
#include "circular_buf.h"

namespace VK {
	namespace Net {
		class SessionMgr;

#pragma warning(push)
#pragma warning(disable:4251)
		// represents a session between server and client
		class NET_EXPORT Session final : public std::enable_shared_from_this<Session> {
#pragma warning(pop)
		public:
			explicit Session(session_handler_ptr_t handler);
			~Session();

			bool prepare();
			bool close();

			void connect(const char* ip_or_host, int port);

			int get_id() const;
			uv_tcp_t& get_stream();
			cbuf_ptr_t get_read_cbuf() const;
			session_handler_ptr_t get_handler() const;

#pragma region("Message patterns")
			// req/rep pattern
			void request(cbuf_ptr_t pcb, req_cb_t req_cb);
			// push pattern
			void push(cbuf_ptr_t pcb, send_cb_t cb);
			// ping
			void ping();
			// pong
			void pong(pp_serial_t serial);
			// sub
			void sub(const char * subject);
			// unsub
			void unsub(const char * subject);
#pragma endregion("Message patterns")
			
			// send data through underline stream
			// donot call unless you know what you are doing...
			template <typename ... Args>
			void send(cbuf_ptr_t pcb, send_cb_t cb, Args ... args);
			void send(cbuf_ptr_t pcb, send_cb_t cb);
			void send(std::vector<cbuf_ptr_t>& pcbs, send_cb_t cb);

			// post read request
			// normally requested by a TcpServer
			// TcpClient will post it when session established automatically
			bool post_read_req();

			// return seconds
			time_t get_elapsed_since_last_response() const;
			uint8_t get_keep_alive_counter() const;

			// get remote peer ip
			std::string get_peer_ip() const;

		private:
			// dispatch packages
			bool dispatch(ssize_t nread);

			// response
			void response(error_no_t en, serial_t serial, cbuf_ptr_t pcb, send_cb_t cb);

			// handle messages
			void on_message(cbuf_ptr_t pcb);
			void dispatch_message(cbuf_ptr_t pcb);

			// response
			void on_response(serial_t serial, cbuf_ptr_t pcb);
			
			// 
			bool enqueue_req(req_cb_t cb);

			// stop read
			void stop_read();
			void get_addr_info(const char* host, int port, std::function<void(bool, sockaddr*)> cb) const;
			void connect(sockaddr* addr);

		private:
			// underline uv stream
			read_req_t m_readReq;
			
			// session id
			// unique in current process
			uint32_t m_id;

			// session id seed
			static session_id_t g_sessionId;

			// serial seed
			serial_t m_serial = 0;
			pp_serial_t m_pp_serial = 0;

			// tmp len
			uint16_t pack_desired_size = 0;

			// last ping time
			time_t m_lastResponseTime;
			uint8_t m_keepAliveCounter;

#pragma warning(push)
#pragma warning(disable:4251)
			// read buf
			cbuf_ptr_t m_cbuf;

			// handler
			session_handler_ptr_t m_handler;

			// huge package slices
			std::vector<cbuf_ptr_t> m_pcbArray;

			// request pool
			std::map<serial_t, req_cb_t> m_requestsPool;
#pragma warning(pop)
		};

		template <typename ... Args>
		void Session::send(cbuf_ptr_t pcb, send_cb_t cb, Args ... args) {
			do {
				if (pcb == nullptr)
					pcb = alloc_cbuf(0);

				auto slices = std::move(pack(pcb, args...));
				auto cnt = slices.size();
				if (cnt == 0)
					break;
				
				send(slices, cb);
				return;
			}
			while (0);

			if (cb)
				cb(false, shared_from_this());
		}
	}
}

#endif