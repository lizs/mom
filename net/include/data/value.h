// lizs 2017.3.11
#pragma once
#include "observable/signal.h"
#include "any.h"

namespace VK {
	typedef uint16_t val_id_t;

	// 值
	// 基于Signal实现
	class Value {
		val_id_t m_id;
		any m_val;
		Signal<void(val_id_t, const Value&)> m_signal;

	public:
		typedef std::function<void(val_id_t, const Value&)> slot_t;

		Value() : m_id(0), m_val() { }

		template <typename ValueType>
		explicit Value(val_id_t id, const ValueType& Value)
			: m_id(id), m_val(Value) { }

		template <typename ValueType>
		explicit Value(val_id_t id, ValueType&& Value)
			: m_id(id), m_val(std::forward<ValueType&&>(Value)) { }
		
		template <class ValueType>
		Value& operator=(ValueType&& val) {
			if (m_val.type() != typeid(val))
				throw std::exception("invalid type!");

			any(std::forward<ValueType&&>(val)).swap(m_val);
			m_signal(m_id, *this);
			return *this;
		}

		bool empty() const {
			return m_val.empty();
		}

		val_id_t get_id() const {
			return m_id;
		}

		void clear() {
			m_id = 0;
			m_val.clear();
		}

		// signal
		connector_id_t conn(slot_t slot) {
			return m_signal.conn(slot);
		}

		void disconn(connector_id_t cid) {
			m_signal.disconn(cid);
		}

		void disconn_all() {
			m_signal.disconn_all();
		}

		// cast
		template <typename ValueType>
		const ValueType& to() const {
			return any_cast<const ValueType&>(m_val);
		}
	};
}
