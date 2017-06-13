#pragma once
#include "defines.h"
namespace VK {
	namespace Net {
		// net专用单例类
		template <typename T>
		class NET_API _singleton_ {
		public:
			static T& instance();

		private:
			_singleton_<T>();
			~_singleton_<T>();
		};
	}
}
