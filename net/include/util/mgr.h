// lizs 2017.6.8

#ifndef MOM_MGR_H
#define MOM_MGR_H

#include <functional>
#include <vector>
#include <map>
#include "observable/signal.h"

namespace VK {
	template <typename TKey, typename TVal>
	class Mgr {
	public:
		typedef TKey key_t;
		typedef TKey& key_ref_t;
		typedef const TKey& const_key_ref_t;

		typedef TVal value_t;
		typedef TVal& value_ref_t;
		typedef const TVal& const_value_ref_t;
		typedef TVal&& value_rref_t;
		typedef const TVal&& const_value_rref_t;
		typedef TVal* value_ptr_t;

		//typedef 
		Signal<void(const_value_ref_t)> EventAdded;
		Signal<void(const_value_ref_t)> EventRemoved;

#pragma region "iterator"
		auto begin() {
			return m_items.begin();
		}

		auto end() {
			return m_items.end();
		}

		auto begin() const {
			return m_items.begin();
		}

		auto end() const {
			return m_items.end();
		}
#pragma endregion "iterator"

		void for_each(std::function<void(value_ref_t)> function) {
			for (auto& pair : m_items) {
				function(pair.second);
			}
		}

		bool add(const_key_ref_t id) {
			if (contains(id))
				return false;

			value_ref_t item = m_items[id];
			item;
			return true;
		}

		bool add(const_key_ref_t id, const_value_ref_t val) {
			if (contains(id))
				return false;

			value_ref_t item = m_items[id];
			item = val;

			EventAdded(item);
			return true;
		}

		bool add(const_key_ref_t id, const_value_rref_t val) {
			if (contains(id))
				return false;

			value_ref_t item = m_items[id];
			item = std::move(val);

			EventAdded(item);
			return true;
		}

		bool remove(const_key_ref_t id) {
			auto it = m_items.find(id);
			if (it == m_items.end())
				return false;

			EventRemoved(it->second);
			m_items.erase(it);
			return true;
		}

		bool clear() {
			for (auto& item : m_items) {
				EventRemoved(item.second);
			}

			m_items.clear();
			return true;
		}

		void remove(std::function<bool(const key_t, const const_value_ref_t)> predicate) {
			for (auto it = m_items.begin(); it != m_items.end();) {
				if (predicate(it->first, it->second)) {
					EventRemoved(it->second);
					m_items.erase(it++);
				}
				else {
					++it;
				}
			}
		}

		void remove(std::function<bool(const_value_ref_t)> predicate) {
			for (auto it = m_items.begin(); it != m_items.end();) {
				if (predicate(it->second)) {
					EventRemoved(it->second);
					m_items.erase(it++);
				}
				else {
					++it;
				}
			}
		}

		std::vector<value_t> get(std::function<bool(const_value_ref_t)> predicate) {
			std::vector<value_t> ret;
			for (auto& pair : m_items) {
				if (predicate(pair.second)) {
					ret.push_back(pair.second);
				}
			}

			return std::move(ret);
		}

		std::vector<value_t> values() {
			std::vector<value_t> ret;
			for (auto& pair : m_items) {
				ret.push_back(pair.second);
			}

			return std::move(ret);
		}

		std::vector<value_ptr_t> value_ptrs() {
			std::vector<value_ptr_t> ret;
			for (auto& pair : m_items) {
				ret.push_back(&pair.second);
			}

			return std::move(ret);
		}

		value_ptr_t get_random() {
			if (m_items.empty())
				return nullptr;

			auto step = rand() % m_items.size();
			auto it = m_items.begin();
			advance(it, step);
			return &it->second;
		}

		value_ptr_t get_first() {
			if (m_items.empty())
				return nullptr;

			return &(m_items.begin()->second);
		}

		value_ptr_t get_first(std::function<bool(const_value_ref_t)> predicate) {
			for (auto& pair : m_items) {
				if (predicate(pair.second)) {
					return &pair.second;
				}
			}

			return nullptr;
		}

		value_ref_t operator[](const_key_ref_t id) {
			if (contains(id)) {
				return m_items[id];
			}

			value_ref_t item = m_items[id];
			EventAdded(item);

			return item;
		}

		value_ptr_t get(const_key_ref_t id) {
			auto it = m_items.find(id);
			if (m_items.end() != it) {
				return &(it->second);
			}

			return nullptr;
		}

		bool contains(const_key_ref_t id) {
			return m_items.find(id) != m_items.end();
		}

		bool contains(std::function<bool(const_value_ref_t)> predicate) {
			auto it = std::find_if(m_items.begin(), m_items.end(), [](auto it) {
				                       return predicate(it->second);
			                       });

			return it != m_items.end();
		}

	private:
		std::map<key_t, value_t> m_items;
	};
}

#endif
