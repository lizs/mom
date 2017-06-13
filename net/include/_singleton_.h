#pragma once
#include "defines.h"
namespace VK {
	namespace Net {
		// netר�õ�����
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
