#pragma once
#include "singleton.h"

namespace VK {
	namespace Net {
		template <typename T>
		T& Singleton<T>::instance() {
			static T instance;
			return instance;
		}

		template <typename T>
		Singleton<T>::Singleton() {}

		template <typename T>
		Singleton<T>::~Singleton() {}
	}
}
