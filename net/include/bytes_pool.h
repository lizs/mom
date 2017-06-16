// lizs 2017.3.6
#pragma once
#include "defines.h"
#include "mem_pool.h"

namespace VK {
	namespace Net {
		// bytes 池
		class NET_EXPORT BytesPool {
			template <cbuf_len_t Capacity>
			struct bytes_t {
				enum _ {
					Capacity = Capacity,
				};

				char bytes[Capacity];
			};

#pragma warning(push)
#pragma warning(disable:4251)
			MemoryPool<bytes_t<32>> m_32_pool;
			MemoryPool<bytes_t<64>> m_64_pool;
			MemoryPool<bytes_t<128>> m_128_pool;
			MemoryPool<bytes_t<256>> m_256_pool;
			MemoryPool<bytes_t<512>> m_512_pool;
			MemoryPool<bytes_t<1024>> m_1024_pool;
			MemoryPool<bytes_t<2048>> m_2048_pool;
			MemoryPool<bytes_t<4096>> m_4096_pool;
			MemoryPool<bytes_t<8192>> m_8192_pool;
#pragma warning(pop)

		public:
			char* alloc(cbuf_len_t size);
			void dealloc(char* buf, cbuf_len_t size);

		private:
			// 是否为2的幂
			static bool is_power_of_2(cbuf_len_t v);
			// 对齐：最小的不小于size的2的幂
			static cbuf_len_t min_pow_of_2_non_less(cbuf_len_t v);
			// 最大的不大于size的2的幂
			static cbuf_len_t max_pow_of_2_non_greater(cbuf_len_t v);
		};
	}
}
