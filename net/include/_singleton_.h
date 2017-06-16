#pragma once
#include "defines.h"

namespace VK {
	// net专用单例类
	template <typename T>
	class NET_EXPORT _singleton_ {
	public:
		static T& instance();

	protected:
		_singleton_<T>();
		~_singleton_<T>();
	};
}
