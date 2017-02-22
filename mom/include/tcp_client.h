// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include "bull.h"
#include "scheduler.h"

namespace Bull {
	template <typename T>
	class TcpClient {

		const uint64_t MAX_RECONN_DELAY = 32000;
		const uint64_t DEFAULT_RECONN_DELAY = 1000;

	public:
		TcpClient(const char* ip, int port,
		          std::function<void()> open_cb = nullptr,
		          std::function<void()> close_cb = nullptr,
		          bool auto_reconnect_enabled = true);
		virtual ~TcpClient();
		int startup();
		int shutdown();
		int write(const char* data, u_short size, std::function<void(int)> cb) const;

	private:
		void reconnect();
		void reconnect(uint64_t delay);
		void set_reconn_delay(uint64_t delay);
		void double_reonn_delay();

		Scheduler m_scheduler;
		T* m_session;

		bool m_autoReconnect = true;
		std::string m_ip;
		int m_port;

		uint64_t m_reconnDelay = DEFAULT_RECONN_DELAY;
	};

	template <typename T>
	TcpClient<T>::TcpClient(const char* ip, int port,
	                        std::function<void()> open_cb,
	                        std::function<void()> close_cb,
	                        bool auto_reconnect_enabled) : m_session(nullptr), m_autoReconnect(auto_reconnect_enabled), m_ip(ip), m_port(port) {
		m_session = new T([=, this](int status, T* session) {
			                  if (status) {
				                  if (m_autoReconnect) {
					                  reconnect();
				                  }
			                  }
			                  else if (open_cb) {
				                  set_reconn_delay(DEFAULT_RECONN_DELAY);
				                  open_cb();

								  m_session->post_read_req();
			                  }
		                  }, [=, this](T* session) {
			                  set_reconn_delay(DEFAULT_RECONN_DELAY);
			                  if (close_cb) {
				                  close_cb();

				                  if (m_autoReconnect) {
					                  reconnect();
				                  }
			                  }
		                  });
	}

	template <typename T>
	TcpClient<T>::~TcpClient() {}

	template <typename T>
	int TcpClient<T>::startup() {
		uv_loop_t* loop;
		int r;

		loop = uv_default_loop();

		// session start
		r = m_session->prepare();
		if(r) {
			LOG_UV_ERR(r);
			return r;
		}

		// connect
		r = m_session->connect(m_ip.c_str(), m_port);
		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}
		
#if MONITOR_ENABLED
		// performance monitor
		m_scheduler.invoke(1000, 1000, []() {
			LOG("Read : %llu /s Write : %llu", Monitor::g_readed, Monitor::g_wroted);
			Monitor::g_readed = 0;
			Monitor::g_wroted = 0;
		});
#endif

		// main loop
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
		int r = m_session->prepare();
		if (r) {
			LOG_UV_ERR(r);
		}	
		
		double_reonn_delay();
		m_scheduler.invoke(m_reconnDelay, [this]() {
			                   int r = m_session->connect(m_ip.c_str(), m_port);
			                   if (r) {
				                   LOG_UV_ERR(r);
			                   }
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
	int TcpClient<T>::shutdown() {
		int r = m_session->close();
		if (r) {
			LOG_UV_ERR(r);
			return 1;
		}

		return r;
	}

	template <typename T>
	int TcpClient<T>::write(const char* data, u_short size, std::function<void(int)> cb) const {
		return m_session->write(data, size, cb);
	}

}
