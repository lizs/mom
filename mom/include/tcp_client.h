// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include "bull.h"
#include "scheduler.h"

namespace Bull {
	template <typename T>
	class TcpClient {
		typedef struct {
			uv_connect_t req;
			bool* established_flag;
			void* tcp_client;
			std::function<void(int)> cb;
		} connect_req_t;

		const uint64_t MAX_RECONN_DELAY = 32000;
		const uint64_t DEFAULT_RECONN_DELAY = 1000;

	public:
		TcpClient(const char* ip, int port, bool auto_reconnect_enabled = true);
		virtual ~TcpClient();
		int connect(std::function<void(int)> cb = nullptr);
		void reconnect();
		void reconnect(uint64_t delay);
		void set_reconn_delay(uint64_t delay);
		int close();
		int write(const char* data, u_short size, std::function<void(int)> cb) const;

	private:
		void double_reonn_delay();

		Scheduler m_scheduler;
		T* m_session;

		bool m_autoReconnect = true;
		bool m_established = false;
		std::string m_ip;
		int m_port;

		std::function<void(int)> m_connectedCallback;
		uint64_t m_reconnDelay = DEFAULT_RECONN_DELAY;
	};

	template <typename T>
	TcpClient<T>::TcpClient(const char* ip, int port, bool auto_reconnect_enabled) :
		m_session(nullptr), m_autoReconnect(auto_reconnect_enabled), m_ip(ip),
		m_port(port) {}

	template <typename T>
	TcpClient<T>::~TcpClient() {}

	template <typename T>
	int TcpClient<T>::connect(std::function<void(int)> cb) {
		struct sockaddr_in addr;
		uv_loop_t* loop;
		int r;

		// ensure do not cover previous non-empty cb
		if (cb != nullptr)
			m_connectedCallback = cb;

		ASSERT(m_session == nullptr);
		m_session = new T();
		loop = uv_default_loop();

		r = uv_ip4_addr(m_ip.c_str(), m_port, &addr);
		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		r = uv_tcp_init(loop, &m_session->get_stream());
		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		auto cr = new connect_req_t();
		cr->cb = cb;
		cr->tcp_client = this;
		cr->established_flag = &m_established;

		r = uv_tcp_connect(&cr->req,
			&m_session->get_stream(),
			reinterpret_cast<const sockaddr*>(&addr),
			[](uv_connect_t* ct, int status) {
			auto cr = reinterpret_cast<connect_req_t*>(ct);
			if (status) {
				LOG_UV_ERR(status);
				if (cr->cb) {
					cr->cb(status);
				}

				reinterpret_cast<TcpClient<T>*>(cr->tcp_client)->reconnect();

				delete cr;
				return;
			}

			*cr->established_flag = true;
			if (cr->cb) {
				cr->cb(status);
			}

			delete cr;
			LOG("Established!");
		});

		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		r = uv_run(loop, UV_RUN_DEFAULT);
		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		MAKE_VALGRIND_HAPPY();
		return 0;
	}

	template <typename T>
	void TcpClient<T>::double_reonn_delay() {
		m_reconnDelay *= 2;
		if (m_reconnDelay > MAX_RECONN_DELAY)
			m_reconnDelay = MAX_RECONN_DELAY;
	}

	template <typename T>
	void TcpClient<T>::reconnect() {
		double_reonn_delay();

		if (m_session) {
			delete m_session;
			m_session = nullptr;
		}

		m_scheduler.invoke(m_reconnDelay, [this]() {
			connect();
		});
	}

	template <typename T>
	void TcpClient<T>::reconnect(uint64_t delay) {
		set_reconn_delay(delay);
		reconnect();
	}

	template <typename T>
	void TcpClient<T>::set_reconn_delay(uint64_t delay) {
		m_reconnDelay = delay;
	}

	template <typename T>
	int TcpClient<T>::close() {
		if (!m_established) return 0;
		if (m_session == nullptr) return 0;

		m_established = false;
		int r = m_session->close();
		m_session = nullptr;

		if (m_autoReconnect) {
			set_reconn_delay(DEFAULT_RECONN_DELAY);
			reconnect();
		}

		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		return r;
	}

	template <typename T>
	int TcpClient<T>::write(const char* data, u_short size, std::function<void(int)> cb) const {
		if (m_session == nullptr || !m_established) {
			LOG("Tcp write error, session not established\n");
			return 1;
		}

		return m_session->write(data, size, cb);
	}

}
