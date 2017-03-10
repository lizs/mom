#include "bytes_pool.h"
#include <stdexcept>

namespace VK {
	namespace Net {

		char* BytesPool::alloc(cbuf_len_t size) {
			// 对齐
			auto alignment = align_bottom(size);
			switch (alignment) {
				case 2:
				case 4:
				case 6:
				case 8:
				case 16:
				case 32:
					return reinterpret_cast<char*>(m_32_pool.alloc());
				case 64:
					return reinterpret_cast<char*>(m_64_pool.alloc());
				case 128:
					return reinterpret_cast<char*>(m_128_pool.alloc());
				case 256:
					return reinterpret_cast<char*>(m_256_pool.alloc());
				case 512:
					return reinterpret_cast<char*>(m_512_pool.alloc());
				case 1024:
					return reinterpret_cast<char*>(m_1024_pool.alloc());
				case 2048:
					return reinterpret_cast<char*>(m_2048_pool.alloc());
				case 4096:
					return reinterpret_cast<char*>(m_4096_pool.alloc());
				case 8192:
					return reinterpret_cast<char*>(m_8192_pool.alloc());

				default:
					throw std::runtime_error("BytesPool::alloc, invalid size!");
			}
		}

		void BytesPool::dealloc(char* buf, cbuf_len_t size) {
			// 对齐
			auto alignment = align_bottom(size);
			switch (alignment) {
				case 2:
				case 4:
				case 6:
				case 8:
				case 16:
				case 32:
					return m_32_pool.dealloc(reinterpret_cast<bytes_t<32>*>(buf));
				case 64:
					return m_64_pool.dealloc(reinterpret_cast<bytes_t<64>*>(buf));
				case 128:
					return m_128_pool.dealloc(reinterpret_cast<bytes_t<128>*>(buf));
				case 256:
					return m_256_pool.dealloc(reinterpret_cast<bytes_t<256>*>(buf));
				case 512:
					return m_512_pool.dealloc(reinterpret_cast<bytes_t<512>*>(buf));
				case 1024:
					return m_1024_pool.dealloc(reinterpret_cast<bytes_t<1024>*>(buf));
				case 2048:
					return m_2048_pool.dealloc(reinterpret_cast<bytes_t<2048>*>(buf));
				case 4096:
					return m_4096_pool.dealloc(reinterpret_cast<bytes_t<4096>*>(buf));
				case 8192:
					return m_8192_pool.dealloc(reinterpret_cast<bytes_t<8192>*>(buf));

				default:
					throw std::runtime_error("BytesPool::dealloc_cbuf, invalid size!");
			}
		}

		bool BytesPool::is_power_of_2(cbuf_len_t v) {
			// 参考 ：http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
			// 0 isn't a power of 2
			return v && !(v & (v - 1));
		}

		cbuf_len_t BytesPool::align_bottom(cbuf_len_t v) {
			// 参考 ：http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
			v--;
			v |= v >> 1;
			v |= v >> 2;
			v |= v >> 4;
			v |= v >> 8;
			v++;

			return v;
		}

		//cbuf_len_t BytesPool::align_top(cbuf_len_t v) {
		//	// 参考 ：http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
		//	//			v--;
		//	//			v |= v >> 1;
		//	//			v |= v >> 2;
		//	//			v |= v >> 4;
		//	//			v |= v >> 8;
		//	//			v |= v >> 16;
		//	//			v++;

		//	// 简单明了的方式
		//	return static_cast<cbuf_len_t>(log(v) - 1);
		//}
	}
}
