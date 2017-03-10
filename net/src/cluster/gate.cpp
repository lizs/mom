#include "cluster/gate.h"
#include "session.h"

namespace VK {
	namespace Cluster {
		using namespace Net;

		Gate::Gate(node_id_t nid, const char* ip, int port) : MomServer(nid, ip, port) { }

		Gate::~Gate() {}

		void Gate::on_request(session_t* session, cbuf_ptr_t pcb, req_cb_t rcb) {
			auto route = pcb->get_head_ptr<req_header_t>();
			route->sid = session->get_id();

			MomServer::on_request(session, pcb, rcb);
		}
	}
}
