// lizs 2017.6.16
#pragma once
#include "defines.h"
namespace VK {
	namespace Net {
		// �ڵ�ӿ�
		// ����PUB/SUB
		struct IPeer {
		protected:
			~IPeer() {}

		public:
			// ����sub
			virtual void on_sub(session_ptr_t session, const std::string & subject) = 0;
			// ����pub
			virtual void pub(const std::string & subject, cbuf_ptr_t pcb) = 0;
		};
	}
}
