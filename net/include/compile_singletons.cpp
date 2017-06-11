#include "singleton.inl"
#include "mem_pool.h"
#include "monitor.h"
#include "bytes_pool.h"

namespace VK {
	namespace Net {
		// singletons
		void _make_singleton_single() {
			Singleton<Monitor>::instance();
			Singleton<BytesPool>::instance();
			Singleton<MemoryPool<CircularBuf>>::instance();
			Singleton<MemoryPool<write_req_t, 1>>::instance();
		}
	}
}