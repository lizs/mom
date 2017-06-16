// lizs 2017.3.11
#pragma once
#include <functional>
#include <map>

namespace VK {
	typedef uint32_t connector_id_t;
	constexpr connector_id_t INVALID_CONNECTOR_ID = 0;

	template <typename T>
	class Signal;

	template <typename R, typename... Params>
	class Signal<R(Params ...)> {
	public:
		typedef std::function<R(Params ...)> slot_t;

		connector_id_t conn(slot_t slot) {
			if (slot == nullptr) return INVALID_CONNECTOR_ID;
			++m_seed;
			m_slots[m_seed] = slot;
			return m_seed;
		}

		void disconn(connector_id_t cid) {
			auto it = m_slots.find(cid);
			if (it != m_slots.end()) {
				m_slots.erase(it);
			}
		}

		void disconn_all() {
			m_slots.clear();
		}

		void operator()(Params ... args) const {
			for (auto it = m_slots.begin(); it != m_slots.end(); ++it) {
				(it->second)(args ...);
			}
		}

		void emit(Params ... args) const {
			operator()(args ...);
		}

		bool empty() const {
			return m_slots.empty();
		}

	private:
		std::map<connector_id_t, slot_t> m_slots;
		connector_id_t m_seed = 0;
	};
}
