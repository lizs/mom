// lizs 2017.3.11
#pragma once
#include "observable/signal.h"
#include <memory>
#include "value.h"

namespace VK {
	// 属性包
	template <typename T>
	class Property {
	public:
		typedef typename Value<T>::slot_t slot_t;

		Property(const std::initializer_list<T>& pids, const T& init_val) {
			for (auto pid : pids) {
				auto pval = std::make_shared<Value<T>>(pid, init_val);
				pval->conn(std::bind(&Property::on_value_changed, this, std::placeholders::_1, std::placeholders::_2));
				m_values[pid] = pval;
			}
		}

		void set(val_id_t pid, const T& value) {
			auto it = m_values.find(pid);
			if (it == m_values.end())return;
			*(it->second) = value;
		}

		const T& get(val_id_t pid) {
			return m_values[pid]->get();
		}

		bool try_get(val_id_t pid, T& out) {
			auto it = m_values.find(pid);
			if (it == m_values.end()) return false;
			out = it->second->get();
			return true;
		}

		connector_id_t conn_all(slot_t handler) {
			if (handler == nullptr) return INVALID_CONNECTOR_ID;
			++m_seed;
			m_subscribers[m_seed] = handler;
			return m_seed;
		}

		void disconn_all(connector_id_t cid) {
			auto it = m_subscribers.find(cid);
			if (it == m_subscribers.end()) return;
			m_subscribers.erase(it);
		}

		connector_id_t conn(val_id_t pid, slot_t handler) {
			auto it = m_values.find(pid);
			if (it == m_values.end())
				return INVALID_CONNECTOR_ID;

			return it->second->conn(handler);
		}

		void disconn(val_id_t pid, connector_id_t cid) {
			auto it = m_values.find(pid);
			if (it == m_values.end())
				return;

			it->second->disconn(cid);
		}

	private:
		void on_value_changed(val_id_t vid, const T& val) {
			for (auto& kv : m_subscribers) {
				kv.second(vid, val);
			}
		}

	private:
		std::map<val_id_t, std::shared_ptr<Value<T>>> m_values;

		// 包内所有属性监听
		std::map<connector_id_t, slot_t> m_subscribers;
		connector_id_t m_seed = INVALID_CONNECTOR_ID;
	};
}
