#pragma once
namespace VK {

	namespace Net {
		template <typename T>
		class Singleton {
		public:
			static T* instance();
			static void destroy();

		private:
			Singleton<T>();
			~Singleton<T>();

			static T* _instance;
		};

		template <typename T>
		T* Singleton<T>::_instance = nullptr;

		template <typename T>
		T* Singleton<T>::instance() {
			if (_instance == nullptr)
				_instance = new T();
			return _instance;
		}

		template <typename T>
		void Singleton<T>::destroy() {
			if (_instance != nullptr) {
				delete _instance;
				_instance = nullptr;
			}
		}

		template <typename T>
		Singleton<T>::Singleton() {}

		template <typename T>
		Singleton<T>::~Singleton() {}
	}
}
