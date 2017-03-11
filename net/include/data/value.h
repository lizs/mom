// lizs 2017.3.11
#pragma once
#include "observable/signal.h"

namespace VK {
	typedef uint16_t val_id_t;

	// 值
	// 基于Signal实现
	template <typename T>
	class Value {
	public:
		typedef std::function<void(val_id_t, const T&)> slot_t;

		explicit Value(val_id_t vid, const T& val) {
			m_id = vid;
			m_value = val;
		}

		Value(Value<T>& other) {
			m_id = other.m_id;
			m_value = other.m_value;
		}

		connector_id_t conn(slot_t slot) {
			return m_signal.conn(slot);
		}

		void disconn(connector_id_t cid) {
			m_signal.disconn(cid);
		}

		void disconn_all() {
			m_signal.disconn_all();
		}

		Value operator=(const T& val) {
			if (m_value != val) {
				m_value = val;
				m_signal(m_id, val);
			}

			return *this;
		}

		const T& get() {
			return m_value;
		}

		val_id_t get_id() const {
			return m_id;
		}

	private:
		val_id_t m_id;
		T m_value;
		Signal<void(val_id_t, const T&)> m_signal;
	};
}
