// lizs 2017.3.4
#pragma once

#include <list>
#include "slot.h"

// �ο�delegateʵ��
namespace VK {
#pragma region("Macros")
#define SIGNAL_IMP \
void connect(slot_t slot) { \
m_slots.push_back(slot); \
} \
 \
void disconnect(slot_t slot) { \
	auto it = std::find(m_slots.begin(), m_slots.end(), slot); \
	if (it != m_slots.end()) { \
		m_slots.erase(it); \
	} \
}\
\
void emit(Params ... args) const {\
	for (auto it = m_slots.begin(); it != m_slots.end();) {\
		(*(it++))(args...);\
	}\
}\
\
bool empty() const {\
	return m_slots.empty();\
}\
\
private:\
	std::list<slot_t> m_slots;
#pragma endregion ("Macros")

	// ����
	template <typename T>
	class Signal2;

	// ��ͨ�����ػ�
	template <typename R, typename... Params>
	class Signal2<R(*)(Params ...)> {
	public:
		typedef slot<R(*)(Params ...)> slot_t;
		SIGNAL_IMP
	};

	// ��Ա�����ػ�
	template <typename T, typename R, typename... Params>
	class Signal2<R(T::*)(Params ...)> {
	public:
		typedef slot<R(T::*)(Params ...)> slot_t;
		SIGNAL_IMP
	};
}
