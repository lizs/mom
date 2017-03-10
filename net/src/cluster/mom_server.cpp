#include "cluster/mom_server.h"
#include "circular_buf.h"
#include "session.h"

namespace VK {
	namespace Cluster {
		session_t* MomServer::get_session_by_nid(node_id_t nid) {
			auto it = m_nodes.find(nid);
			if (it != m_nodes.end()) {
				return m_tcpServer.get_session_mgr().get_session(it->second->sid);
			}

			return nullptr;
		}

		session_t* MomServer::get_first_session_by_ntype(node_type_t ntype) {
			for (auto& kv : m_nodes) {
				if (kv.second->reg.ntype == ntype) {
					return m_tcpServer.get_session_mgr().get_session(kv.second->sid);
				}
			}

			return nullptr;
		}

		std::vector<session_t*> MomServer::get_sessions_by_ntype(node_type_t ntype) {
			std::vector<session_t*> sessions;

			for (auto& kv : m_nodes) {
				if (kv.second->reg.ntype == ntype) {
					auto session = m_tcpServer.get_session_mgr().get_session(kv.second->sid);
					if (session != nullptr)
						sessions.push_back(session);
				}
			}

			return move(sessions);
		}

		bool MomServer::startup() {
			return m_tcpServer.startup();
		}

		bool MomServer::shutdown() {
			m_nodes.clear();
			return m_tcpServer.shutdown();
		}

		void MomServer::broadcast(cbuf_ptr_t pcb) {
			m_tcpServer.broadcast(pcb);
		}

		void MomServer::multicast(cbuf_ptr_t pcb, std::vector<session_t*>& sessions) const {
			m_tcpServer.multicast(pcb, sessions);
		}

		void MomServer::on_request(session_t* session, cbuf_ptr_t pcb, req_cb_t cb) {
			error_no_t me;

			do {
				req_header_t header;
				if (!pcb->get<req_header_t>(header)) {
					me = ME_ReadHeader;
					break;
				}

				// < 0 => internal ops
				if (header.ops < 0) {
					switch (static_cast<MomOps>(header.ops)) {
						case Register2Gate: {
							node_register_t ret;
							if (!pcb->get<node_register_t>(ret, sizeof(req_header_t))) {
								me = ME_ReadRegisterInfo;
							}
							else {
								if (m_nodes.find(ret.nid) != m_nodes.end()) {
									me = ME_NodeIdAlreadyExist;
								}
								else {
									m_nodes[ret.nid] = std::make_shared<node_info_t>(ret, session->get_id());
									m_sid2nid[session->get_id()] = ret.nid;
									me = Success;
								}
							}
							break;
						}

						case Coordinate: {
							coordinate_req_t ret;
							if (!pcb->get<coordinate_req_t>(ret, sizeof(req_header_t))) {
								me = ME_ReadCoordinateInfo;
							}
							else {
								// 获取coordination 会话
								auto tarSession = get_first_session_by_ntype(NT_Coordination);
								if (tarSession == nullptr) {
									// target node not exist
									me = ME_CoordinationServerNotExist;
									break;
								}

								// forward
								tarSession->request(pcb, [cb](error_no_t eno, cbuf_ptr_t pcb) {
									                    cb(eno, pcb);
								                    });

								return;
							}
							break;
						}

						case Register2Coordination: {
							node_register_t ret;
							if (!pcb->get<node_register_t>(ret, sizeof(req_header_t))) {
								me = ME_ReadRegisterInfo;
							}
							else {
								// 获取coordination 会话
								auto tarSession = get_first_session_by_ntype(NT_Coordination);
								if (tarSession == nullptr) {
									// target node not exist
									me = ME_CoordinationServerNotExist;
									break;
								}

								// forward
								tarSession->request(pcb, [cb](error_no_t eno, cbuf_ptr_t pcb) {
									                    cb(eno, pcb);
								                    });

								return;
							}
							break;
						}

						default:
							me = ME_InvlidOps;
							break;
					}

					break;
				}

				// get target session
				auto tarSession = get_session_by_nid(header.nid);
				if (tarSession == nullptr) {
					// target node not exist
					me = ME_TargetNodeNotExist;
					break;
				}

				// forward
				tarSession->request(pcb, [cb](error_no_t eno, cbuf_ptr_t pcb) {
					                    cb(eno, pcb);
				                    });

				return;
			}
			while (false);

			cb(me, pcb);
		}


		void MomServer::on_push(session_t* session, cbuf_ptr_t pcb) {
			error_no_t me = Success;

			auto header = pcb->get_head_ptr<push_header_t>();
			if (header == nullptr) {
				me = ME_ReadHeader;
				return;
			}

			if (header->ops < 0) {
				switch (static_cast<MomOps>(header->ops)) {
					case Broadcast: {
						pcb->move_head(sizeof(push_header_t));
						broadcast_push_t broadcast;
						if (!pcb->read<broadcast_push_t>(broadcast)) {
							me = ME_ReadBroadcastInfo;
							break;
						}

						auto sessions = get_sessions_by_ntype(broadcast.ntype);
						multicast(pcb, sessions);
						return;
					}
					default:
						me = ME_NotImplemented;
						break;
				}

				return;
			}

			// get target session
			auto tarSession = get_session_by_nid(header->nid);
			if (tarSession == nullptr) {
				// target node not exist
				me = ME_TargetSessionNotExist;
				return;
			}

			// forward
			tarSession->push(pcb, nullptr);

			if (me != Success) {
				LOG(mom_str_err(me));
			}
		}

		void MomServer::on_open(bool success, session_t* session) { }

		void MomServer::on_close(session_t* session) {
			auto it = m_sid2nid.find(session->get_id());
			if (it == m_sid2nid.end()) 
				return;

			auto nid = it->second;
			m_sid2nid.erase(it);

			auto it1 = m_nodes.find(nid);
			if (it1 == m_nodes.end()) 
				return;

			auto ntype = it1->second->reg.ntype;
			m_nodes.erase(it1);

			if (ntype != NT_Coordination) {
				// 若非协调服务，则要通知协调服该服务不可用
				notify_coordination(nid);
			}
		}

		void MomServer::notify_coordination(node_id_t nid) {
			auto tarSession = get_first_session_by_ntype(NT_Coordination);
			if (tarSession == nullptr) return;

			auto pcb = alloc_cbuf<node_unregister_t>();
			auto route = pcb->pre_write_head<req_header_t>();
			if (!route)
				return;

			route->ops = UnregisterFromCoordination;
			auto req = pcb->pre_write<node_unregister_t>();
			if (!req)
				return;

			req->nid = nid;
			tarSession->push(pcb, nullptr);
		}
	}
}
