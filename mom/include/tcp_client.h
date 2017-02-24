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
		bool startup();
		bool shutdown();
#pragma region("Message patterns")
		bool request(const char* data, uint16_t size, std::function<void(bool, typename T::circular_buf_t*)> cb);
		bool push(const char* data, uint16_t size, std::function<void(bool)> cb);
#pragma endregion("Message patterns")

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
		m_session = new T([=, this](bool success, T* session) {
			                  if (!success) {
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
	bool TcpClient<T>::startup() {
		// session start
		if (!m_session->prepare()) {
			return false;
		}

		// connect
		if (!m_session->connect(m_ip.c_str(), m_port)) {
			return false;
		}
		
#if MONITOR_ENABLED
		// performance monitor
		m_scheduler.invoke(1000, 1000, []() {
			LOG("Read : %llu /s Write : %llu", Monitor::g_readed, Monitor::g_wroted);
			Monitor::g_readed = 0;
			Monitor::g_wroted = 0;
		});
#endif

		return true;
	}
	
	template <typename T>
	void TcpClient<T>::double_reonn_delay() {
		m_reconnDelay *= 2;
		if (m_reconnDelay > MAX_RECONN_DELAY)
			m_reconnDelay = MAX_RECONN_DELAY;
	}

	template <typename T>
	void TcpClient<T>::reconnect() {
		m_session->prepare();		
		double_reonn_delay();

		m_scheduler.invoke(m_reconnDelay, [this]() {
			                   m_session->connect(m_ip.c_str(), m_port);
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
	bool TcpClient<T>::shutdown() {
		return  m_session->close();
	}

	template <typename T>
	bool TcpClient<T>::request(const char* data, uint16_t size, std::function<void(bool, typename T::circular_buf_t*)> cb) {
		return m_session->request(data, size, cb);
	}

	template <typename T>
	bool TcpClient<T>::push(const char* data, uint16_t size, std::function<void(bool)> cb) {
		return m_session->push(data, size, cb);
	}
}
