// lizs 2017.3.6
#ifndef MOM_BYTES_POOL_H
#define MOM_BYTES_POOL_H

#include "defines.h"

namespace VK {
	namespace Net {
		// bytes pool
		class NET_EXPORT BytesPool {
		public:
			static char* alloc(cbuf_len_t size);
			static void dealloc(char* buf, cbuf_len_t size);

		private:
			static bool is_power_of_2(cbuf_len_t v);
			static cbuf_len_t min_pow_of_2_non_less(cbuf_len_t v);
			static cbuf_len_t max_pow_of_2_non_greater(cbuf_len_t v);
		};
	}
}

#endif