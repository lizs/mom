#pragma once
#include "_singleton_.h"

namespace VK {
	template <typename T>
	T& _singleton_<T>::instance() {
		static T instance;
		return instance;
	}

	template <typename T>
	_singleton_<T>::_singleton_() {}

	template <typename T>
	_singleton_<T>::~_singleton_() {}
}
