// lizs 2017.3.11
#pragma once
#include <functional>
#include <map>

namespace VK {
	typedef uint32_t connector_t;

	template <typename T>
	class Signal;

	template <typename R, typename... Params>
	class Signal<R(Params ...)> {
	public:
		typedef std::function<R(Params ...)> slot_t;

		connector_t connect(slot_t slot) {
			++m_seed;
			m_slots[m_seed] = slot;
			return m_seed;
		}

		void disconnect(connector_t cid) {
			auto it = m_slots.find(cid);
			if (it != m_slots.end()) {
				m_slots.erase(it);
			}
		}

		void disconnect_all() {
			m_slots.clear();
		}

		void operator()(Params ... args) const {
			for (auto it = m_slots.begin(); it != m_slots.end(); ++it) {
				(it->second)(args...);
			}
		}

		void emit(Params ... args) const {
			operator()(args ...);
		}

		bool empty() const {
			return m_slots.empty();
		}

	private:
		std::map<connector_t, slot_t> m_slots;
		connector_t m_seed = 0;
	};
}
