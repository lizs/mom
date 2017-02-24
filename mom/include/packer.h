// author : lizs
// 2017.2.21

#pragma once
#include "session.h"

namespace Bull {
	template <cbuf_len_t Capacity = 64>
	class Handler {
	public:
		enum CircularBufCapacity {
			Value = Capacity,
		};

		void on_push(const char * data, cbuf_len_t len) {
		}

		bool on_request(const char * data, cbuf_len_t len, char ** responseData, cbuf_len_t & responseLen) {
			*responseData = "I'm a response.";
			responseLen = strlen(*responseData);
			return true;
		}
	};
}
