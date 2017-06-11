#pragma once
#include "defines.h"
namespace VK {
	namespace Net {
		template <typename T>
		class NET_API Singleton {
		public:
			static T& instance();

		private:
			Singleton<T>();
			~Singleton<T>();
		};
	}
}
