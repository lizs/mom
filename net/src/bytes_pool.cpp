#include "bytes_pool.h"
#include "mem_pool.h"
#include <stdexcept>

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

		char* BytesPool::alloc(cbuf_len_t size) {
			// 对齐
			auto alignment = min_pow_of_2_non_less(size);
			switch (alignment) {
				case 2:
				case 4:
				case 6:
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
				case 1024:
					return reinterpret_cast<char*>(MP(1024).alloc());
				case 2048:
					return reinterpret_cast<char*>(MP(2048).alloc());
				case 4096:
					return reinterpret_cast<char*>(MP(4096).alloc());
				case 8192:
					return reinterpret_cast<char*>(MP(8192).alloc());

				default:
					throw std::runtime_error("BytesPool::alloc, invalid size!");
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
				case 1024:
					return MP(1024).dealloc(reinterpret_cast<bytes_t<1024>*>(buf));
				case 2048:
					return MP(2048).dealloc(reinterpret_cast<bytes_t<2048>*>(buf));
				case 4096:
					return MP(4096).dealloc(reinterpret_cast<bytes_t<4096>*>(buf));
				case 8192:
					return MP(8192).dealloc(reinterpret_cast<bytes_t<8192>*>(buf));

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
