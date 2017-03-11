#pragma once
#include <list>

namespace VK {
	namespace Net {
		// 内存池
		// 本池中的T数量不超过Capacity
		template <typename T, size_t Capacity = 0>
		class MemoryPool {

#pragma warning(push)
#pragma warning(disable:4624)
			union Slot {
				T element;
				Slot* next;
			};
#pragma warning(pop) 

			Slot* _free_slots = nullptr;
			size_t _slot_cnt = 0;

		public:
			template <typename ... Args>
			T* alloc(Args ... args) {
				if (_free_slots != nullptr) {
					T* result = reinterpret_cast<T*>(_free_slots);
					_free_slots = _free_slots->next;

					new(result) T(args...);
					--_slot_cnt;
					return result;
				}

				auto slot = malloc(sizeof(Slot));
				auto pelement = reinterpret_cast<T*>(slot);
				new(pelement) T(args...);

				return pelement;
			}

			void dealloc(T* p) {
				if (p != nullptr) {
					if (_slot_cnt >= Capacity) {
						p->~T();
						free(p);
					}
					else {
						auto slot = reinterpret_cast<Slot*>(p);
						slot->next = _free_slots;
						_free_slots = slot;
						++_slot_cnt;
					}
				}
			}
		};
	}
}
