
#ifndef MOM_SINGLETON_H
#define MOM_SINGLETON_H

#include "defines.h"
#include <memory>

namespace VK {
#define SINGLETON(T) \
		friend class std::shared_ptr<T>; \
	public:\
		static T& instance() {	\
			static std::shared_ptr<T> _ins;\
			if (!_ins) {			\
				_ins = std::make_shared<T>();\
			}\
			return *_ins;	\
	}\
	//private:	\
		T() = default;\
		~T() = default;


#define CUSTOM_SINGLETON(T) \
		friend class std::shared_ptr<T>; \
	public:\
		static T& instance() {	\
			static std::shared_ptr<T> _ins;\
			if (!_ins) {			\
				_ins = std::make_shared<T>();\
			}\
			return *_ins;	\
	}\
	//private:	\
		T();\
		~T() = default;
}

#endif
