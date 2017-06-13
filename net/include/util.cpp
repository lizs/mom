#include "util.h"

namespace VK {
	namespace Net {
		cbuf_ptr_t alloc_cbuf(cbuf_len_t len) {
			cbuf_ptr_t pcb(_singleton_<cbuf_pool_t>::instance().alloc());
			pcb->reset(len);

			return pcb;
		}

		const char* net_str_err(error_no_t error_no) {
			switch (static_cast<NetError>(error_no)) {
			case Success: return "Success.";
			case NE_Write: return "Write failed.";
			case NE_Read: return "Read failed.";
			case NE_SerialConflict: return "Serial failed.";
			case NE_NoHandler: return "No handler.";
			case NE_ReadErrorNo: return "Read errno failed.";
			case NE_SessionClosed: return "Session closed.";
			case NE_End: return "NE_End.";
			default: return "Unknown";
			}
		}
	}
}