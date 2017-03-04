// lizs 2017.3.2
#pragma once
#include "mom_server.h"

namespace VK {
	namespace Cluster {
		using namespace Net;
		// 网关服务器
		template <typename T>
		class Gate : public MomServer<T> {
			typedef MomServer<T> Base;
		public:
			TYPE_DEFINES(T)
			explicit Gate(node_id_t nid, const char * ip, int port);
			~Gate() override;

		protected:
			void on_request(session_t* session, cbuf_t* pcb, req_cb_t rcb) override;
		};

		template <typename T>
		Gate<T>::Gate(node_id_t nid, const char * ip, int port): MomServer<T>(nid, ip, port) { }

		template <typename T>
		Gate<T>::~Gate() {}

		template <typename T>
		void Gate<T>::on_request(session_t* session, cbuf_t* pcb, req_cb_t rcb) {
			// 标记会话ID
			auto route = pcb->template get_head_ptr<route_t>();
			route->sid = session->get_id();

			Base::on_request(session, pcb, rcb);
		}
	}
}
