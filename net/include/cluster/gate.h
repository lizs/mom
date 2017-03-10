// lizs 2017.3.2
#pragma once
#include "mom_server.h"

namespace VK {
	namespace Cluster {
		using namespace Net;
		// Íø¹Ø·þÎñÆ÷		
		class NET_API Gate : public MomServer {
		public:
			explicit Gate(node_id_t nid, const char * ip, int port);
			~Gate() override;

		protected:
			void on_request(session_t* session, cbuf_ptr_t pcb, req_cb_t rcb) override;
		};
	}
}
