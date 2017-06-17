// lizs 2017.6.16
#pragma once
#include "defines.h"
namespace VK {
	namespace Net {
		// 节点接口
		// 处理PUB/SUB
		struct IPeer {
		protected:
			~IPeer() {}

		public:
			// 处理sub
			virtual void on_sub(session_ptr_t session, const std::string & subject) = 0;
			// 请求pub
			virtual void pub(const std::string & subject, cbuf_ptr_t pcb) = 0;
		};
	}
}
