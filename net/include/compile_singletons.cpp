#include "defines.h"
#include "mem_pool.h"
#include "monitor.h"

namespace VK {
	namespace Net {
		// 仅为了导出这些单例
		void _compile_singletons_() {
			Monitor::instance();
			MemoryPool<CircularBuf>::instance();
			MemoryPool<write_req_t>::instance();

			//MemoryPool<close_req_t>::instance();
			MemoryPool<connect_req_t>::instance();
			MemoryPool<getaddr_req_t>::instance();

			Logger::instance();
		}
	}
}
