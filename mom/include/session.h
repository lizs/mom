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
		explicit Session();
		~Session();

		void set_host(SessionMgr<Session<T>>* mgr);
		int get_id() const;
		uv_tcp_t& get_stream();
		CircularBuf<T::CircularBufCapacity::Value>& get_read_cbuf();
		int close();

		// write data through underline stream
		int write(const char* data, u_short size, std::function<void(int)> cb);

#if MONITOR_ENABLED
		// performance monitor
		static const uint64_t& get_readed() {
			return g_readed;
		}

		static void set_readed(uint64_t cnt) {
			g_readed = cnt;
		}

		static const uint64_t& get_wroted() {
			return g_wroted;
		}

		static void set_wroted(uint64_t cnt) {
			g_wroted = cnt;
		}
#endif

	private:
		// dispatch packages
		bool Session::dispatch(ssize_t nread);

		T m_packer;
		SessionMgr<Session<T>>* m_host;
		CircularBuf<T::CircularBufCapacity::Value> m_cbuf;
		uv_tcp_t m_stream;
		uv_shutdown_t m_sreq;

		// session id
		// unique in current process
		uint32_t m_id;

#if MONITOR_ENABLED
		// record packages readed since last reset
		// shared between sessions
		static uint64_t g_readed;
		// record packages wroted since last reset
		// shared between sessions
		static uint64_t g_wroted;
#endif

		typedef struct {
			uv_write_t req;
			uv_buf_t buf;
			std::function<void(int)> cb;
			CircularBuf<T::CircularBufCapacity::Value>* pcb;
		} write_req_t;

		static MemoryPool<write_req_t> g_wrPool;
		static MemoryPool<CircularBuf<64>> g_messagePool;

		// session id seed
		static uint32_t g_id;

		// tmp len
		ushort pack_desired_size = 0;
	};

	template <typename T>
	uint64_t Session<T>::g_readed = 0;
	template <typename T>
	uint32_t Session<T>::g_id = 0;

	template <typename T>
	MemoryPool<typename Session<T>::write_req_t> Session<T>::g_wrPool;
	template <typename T>
	MemoryPool<CircularBuf<64>> Session<T>::g_messagePool;

	template <typename T>
	Session<T>::Session() : m_host(nullptr), m_id(++g_id) { }

	template <typename T>
	Session<T>::~Session() { }

	template <typename T>
	void Session<T>::set_host(SessionMgr<Session<T>>* mgr) {
		m_host = mgr;
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
	CircularBuf<T::CircularBufCapacity::Value>& Session<T>::get_read_cbuf() {
		return m_cbuf;
	}

	template <typename T>
	int Session<T>::close() {
		if (m_host) {
			m_host->remove(this);
			m_host = nullptr;
		}

		int r;
		m_sreq.data = this;
		r = uv_shutdown(&m_sreq, reinterpret_cast<uv_stream_t*>(&m_stream), [](uv_shutdown_t* req, int status) {
			                req->handle->data = req->data;
			                uv_close(reinterpret_cast<uv_handle_t*>(req->handle), [](uv_handle_t* stream) {
				                         auto session = static_cast<Session *>(stream->data);
				                         LOG("Session %d closed!", session->get_id());
				                         delete session;
			                         });
		                });

		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		return r;
	}

	template <typename T>
	int Session<T>::write(const char* data, uint16_t size, std::function<void(int)> cb) {
		auto wr = g_wrPool.newElement();
		wr->cb = cb;

		auto* pcb = g_messagePool.newElement();
		pcb->reset();

		// skip size
		pcb->move_tail(sizeof(uint16_t));

		// do translate
		auto offset = m_packer.pack(*pcb);

		// final data
		pcb->write_binary(const_cast<char*>(data), size);

		// rewrite the package size
		pcb->move_tail(-size - offset - sizeof(uint16_t));
		pcb->write<uint16_t>(size + offset);
		pcb->move_tail(offset + size);

		wr->pcb = pcb;
		wr->buf = uv_buf_init(pcb->get_head(), pcb->get_readable_len());

		int r = uv_write(&wr->req,
		                 reinterpret_cast<uv_stream_t*>(&m_stream),
		                 &wr->buf, 1,
		                 [](uv_write_t* req, int status) {
			                 auto wr = reinterpret_cast<write_req_t*>(req);
			                 if (wr->cb != nullptr) {
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

	template <typename T>
	bool Session<T>::dispatch(ssize_t nread) {
		auto& cbuf = get_read_cbuf();
		cbuf.move_tail(static_cast<ushort>(nread));

		while (true) {
			if (pack_desired_size == 0) {
				// get package len
				if (cbuf.get_len() >= sizeof(ushort)) {
					pack_desired_size = cbuf.template read<ushort>();
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
				// unpack
				auto offset = m_packer.unpack(cbuf);

				// notify message
				m_packer.on_message(cbuf.get_head(), pack_desired_size - offset, this);

				++g_readed;
				cbuf.move_head(pack_desired_size - offset);
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
