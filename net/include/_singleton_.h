#pragma once
#include "defines.h"

namespace VK {
	// netר�õ�����
	template <typename T>
	class NET_EXPORT _singleton_ {
	public:
		static T& instance();

	protected:
		_singleton_<T>();
		~_singleton_<T>();
	};
}
