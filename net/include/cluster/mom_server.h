// lizs 2017.2.27

#pragma once
#include <map>
#include "tcp_server.h"
#include <memory>
#include <vector>

namespace VK {
	namespace Cluster {
		using namespace Net;
		class NET_API MomServer {
		public:
			virtual ~MomServer() {}

			explicit MomServer(node_id_t nid, const char* ip, int port)
				: m_id(nid), m_tcpServer(ip, port,
				                         std::bind(&MomServer::on_open, this, std::placeholders::_1, std::placeholders::_2),
				                         std::bind(&MomServer::on_close, this, std::placeholders::_1),
				                         std::bind(&MomServer::on_request, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
				                         std::bind(&MomServer::on_push, this, std::placeholders::_1, std::placeholders::_2)) {}

			session_t* get_session_by_nid(node_id_t nid);
			session_t* get_first_session_by_ntype(node_type_t ntype);
			std::vector<session_t*> get_sessions_by_ntype(node_type_t ntype);
			bool startup();
			bool shutdown();

			void broadcast(cbuf_ptr_t pcb);
			void multicast(cbuf_ptr_t pcb, std::vector<session_t*>& sessions) const;

		protected:
			virtual void on_request(session_t* session, cbuf_ptr_t pcb, req_cb_t rcb);
			virtual void on_push(session_t* session, cbuf_ptr_t pcb);
			virtual void on_open(bool success, session_t* session);
			virtual void on_close(session_t* session);

			std::map<node_id_t, std::shared_ptr<node_info_t>> m_nodes;
			std::map<session_id_t, node_id_t> m_sid2nid;
			node_id_t m_id;
			TcpServer m_tcpServer;

		private:
			void notify_coordination(node_id_t nid);
		};
	}
}
