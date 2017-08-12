// lizs 2017.3.6
#pragma once
#include "defines.h"

namespace VK {
	namespace Net {
		// bytes ��
		class NET_EXPORT BytesPool {
		public:
			static char* alloc(cbuf_len_t size);
			static void dealloc(char* buf, cbuf_len_t size);

		private:
			// �Ƿ�Ϊ2����
			static bool is_power_of_2(cbuf_len_t v);
			// ���룺��С�Ĳ�С��size��2����
			static cbuf_len_t min_pow_of_2_non_less(cbuf_len_t v);
			// ���Ĳ�����size��2����
			static cbuf_len_t max_pow_of_2_non_greater(cbuf_len_t v);
		};
	}
}
