#include "_singleton_.inl"
#include "mem_pool.h"
#include "monitor.h"
#include "bytes_pool.h"

namespace VK {
	namespace Net {
		// singletons
		void _compile_singletons_() {
			_singleton_<Monitor>::instance();
			_singleton_<BytesPool>::instance();
			_singleton_<MemoryPool<CircularBuf>>::instance();
			_singleton_<MemoryPool<write_req_t>>::instance();

			_singleton_<MemoryPool<close_req_t>>::instance();
			_singleton_<MemoryPool<connect_req_t>>::instance();
			_singleton_<MemoryPool<getaddr_req_t>>::instance();
		}
	}
}
