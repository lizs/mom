
#ifndef MOM_SINGLETON_H
#define MOM_SINGLETON_H

#include "defines.h"
#include <memory>

namespace VK {
#define SINGLETON(T) \
	public:\
		static T& instance() {	\
			static T* _ins;\
			if (!_ins) {			\
				_ins = new T();\
			}\
			return *_ins;	\
	}\
	private:	\
		T() = default;\
		~T() = default;


#define CUSTOM_SINGLETON(T) \
	public:\
		static T& instance() {	\
			static T* _ins;\
			if (!_ins) {			\
				_ins =new T();\
			}\
			return *_ins;	\
	}\
	private:\
		T();\
		~T(){};
}

#endif
