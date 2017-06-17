// lizs 2017.6.17
#pragma once
#include "defines.h"

namespace VK {
	namespace Net {
		struct IHandler {
		protected:
			virtual ~IHandler() {}

		public:
			virtual void on_req(session_ptr_t, cbuf_ptr_t, resp_cb_t) = 0;
			virtual error_no_t on_push(session_ptr_t, cbuf_ptr_t) = 0;
			virtual void on_connected(bool, session_ptr_t) = 0;
			virtual void on_closed(session_ptr_t) = 0;
		};

		struct ISessionHandler : public IHandler {
		protected:
			virtual ~ISessionHandler() {}

		public:
			virtual void on_sub(session_ptr_t, const char*) = 0;
			virtual void on_unsub(session_ptr_t, const char*) = 0;
		};
	}
}
