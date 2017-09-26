#include "bytes_pool.h"
#include "mem_pool.h"
#include <stdexcept>
#include <logger.h>

namespace VK {
	namespace Net {
		template <cbuf_len_t Capacity>
		struct bytes_t {
			enum _ {
				Capacity = Capacity,
			};

			char bytes[Capacity];
		};

#define MP(size) MemoryPool<bytes_t<size>>::instance()
#define K 1024

		char* BytesPool::alloc(cbuf_len_t size) {
			// 对齐
			auto alignment = min_pow_of_2_non_less(size);
			switch (alignment) {
				case 2:
				case 4:
				case 8:
				case 16:
				case 32:
					return reinterpret_cast<char*>(MP(32).alloc());
				case 64:
					return reinterpret_cast<char*>(MP(64).alloc());
				case 128:
					return reinterpret_cast<char*>(MP(128).alloc());
				case 256:
					return reinterpret_cast<char*>(MP(256).alloc());
				case 512:
					return reinterpret_cast<char*>(MP(512).alloc());
				case K:
					return reinterpret_cast<char*>(MP(1024).alloc());
				case 2 * K:
					return reinterpret_cast<char*>(MP(2*K).alloc());
				case 4 * K:
					return reinterpret_cast<char*>(MP(4*K).alloc());
				case 8 * K:
					return reinterpret_cast<char*>(MP(8*K).alloc());
				case 16 * K:
					return reinterpret_cast<char*>(MP(16*K).alloc());

				default:
					LOG_ERROR("Alloc cbuf failed : {} is much too huge than 16K.", size);
					return nullptr;
			}
		}

		void BytesPool::dealloc(char* buf, cbuf_len_t size) {
			// 对齐
			auto alignment = min_pow_of_2_non_less(size);
			switch (alignment) {
				case 2:
				case 4:
				case 6:
				case 8:
				case 16:
				case 32:
					return MP(32).dealloc(reinterpret_cast<bytes_t<32>*>(buf));
				case 64:
					return MP(64).dealloc(reinterpret_cast<bytes_t<64>*>(buf));
				case 128:
					return MP(128).dealloc(reinterpret_cast<bytes_t<128>*>(buf));
				case 256:
					return MP(256).dealloc(reinterpret_cast<bytes_t<256>*>(buf));
				case 512:
					return MP(512).dealloc(reinterpret_cast<bytes_t<512>*>(buf));
				case K:
					return MP(K).dealloc(reinterpret_cast<bytes_t<K>*>(buf));
				case 2 * K:
					return MP(2*K).dealloc(reinterpret_cast<bytes_t<2 * K>*>(buf));
				case 4 * K:
					return MP(4*K).dealloc(reinterpret_cast<bytes_t<4 * K>*>(buf));
				case 8 * K:
					return MP(8*K).dealloc(reinterpret_cast<bytes_t<8 * K>*>(buf));
				case 16 * K:
					return MP(16*K).dealloc(reinterpret_cast<bytes_t<16 * K>*>(buf));

				default:
					throw std::runtime_error("BytesPool::dealloc_cbuf, invalid size!");
			}
		}

		bool BytesPool::is_power_of_2(cbuf_len_t v) {
			// 参考 ：http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
			// 0 isn't a power of 2
			return v && !(v & (v - 1));
		}

		cbuf_len_t BytesPool::min_pow_of_2_non_less(cbuf_len_t v) {
			// 参考 ：http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
			// 思想：将最高位的1拷贝至所有低位
			// 10步
			v--;
			v |= v >> 1;
			v |= v >> 2;
			v |= v >> 4;
			v |= v >> 8;
			v++;

			return v;
		}

		cbuf_len_t BytesPool::max_pow_of_2_non_greater(cbuf_len_t n) {
			// 参考 ：http://stackoverflow.com/questions/53161/find-the-highest-order-bit-in-c
			// 思想： 补齐所有低位的1，然后和右移1位的值做差
			n |= (n >> 1);
			n |= (n >> 2);
			n |= (n >> 4);
			n |= (n >> 8);
			return n ^ (n >> 1);
		}
	}
}
