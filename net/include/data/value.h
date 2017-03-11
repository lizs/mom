// lizs 2017.3.11
#pragma once
#include "observable/signal.h"

namespace VK {
	// 值
	// 基于Signal实现
	template <typename T>
	class Value {
	public:
		typedef std::function<void(const T&)> slot_t;

		Value(const T & val) {
			m_value = val;
		}

		connector_t connect(slot_t slot) {
			return m_signal.connect(slot);
		}

		void disconnect(connector_t cid) {
			m_signal.disconnect(cid);
		}

		void disconnect_all() {
			m_signal.disconnect_all();
		}

		Value operator=(const T& val) {
			if (m_value != val) {
				m_value = val;
				m_signal(val);
			}

			return *this;
		}

	private:
		T m_value;
		Signal<void(const T&)> m_signal;
	};
}
