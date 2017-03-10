#pragma once
namespace VK {
	namespace Net {
		template <typename T>
		class Singleton {
		public:
			static T& instance();

		private:
			Singleton<T>();
			~Singleton<T>();
		};

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
