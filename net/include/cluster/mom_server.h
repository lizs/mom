// lizs 2017.2.27

#pragma once
#include <map>
#include "tcp_server.h"
#include "proto.h"
#include <memory>

namespace VK {
	namespace Cluster {
		using namespace Net;
		template <typename T>
		class MomServer {
		public:
			TYPE_DEFINES(T)

			virtual ~MomServer() {}

			explicit MomServer(node_id_t nid, const char* ip, int port)
				: m_id(nid), m_tcpServer(ip, port,
				                         std::bind(&MomServer<T>::on_open, this, std::placeholders::_1, std::placeholders::_2),
				                         std::bind(&MomServer<T>::on_close, this, std::placeholders::_1),
				                         std::bind(&MomServer<T>::on_request, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
				                         std::bind(&MomServer<T>::on_push, this, std::placeholders::_1, std::placeholders::_2)) {}

			session_t* get_session_by_nid(node_id_t nid);
			session_t* get_session_by_ntype(node_type_t ntype);
			bool startup();
			bool shutdown();

		protected:
			virtual void on_request(session_t* session, cbuf_t* pcb, req_cb_t rcb);
			virtual void on_push(session_t* session, cbuf_t* pcb);
			virtual void on_open(bool success, session_t* session);
			virtual void on_close(session_t* session);

			std::map<node_id_t, std::shared_ptr<node_info_t>> m_nodes;
			node_id_t m_id;
			TcpServer<session_t> m_tcpServer;
		};

		template <typename T>
		typename MomServer<T>::session_t* MomServer<T>::get_session_by_nid(node_id_t nid) {
			auto it = m_nodes.find(nid);
			if (it != m_nodes.end()) {
				return m_tcpServer.get_session_mgr().get_session(it->second->sid);
			}

			return nullptr;
		}

		template <typename T>
		typename MomServer<T>::session_t* MomServer<T>::get_session_by_ntype(node_type_t ntype) {
			for (auto& kv : m_nodes) {
				if (kv.second->reg.ntype == ntype) {
					return m_tcpServer.get_session_mgr().get_session(kv.second->sid);
				}
			}

			return nullptr;
		}

		template <typename T>
		bool MomServer<T>::startup() {
			return m_tcpServer.startup();
		}

		template <typename T>
		bool MomServer<T>::shutdown() {
			m_nodes.clear();
			return m_tcpServer.shutdown();
		}

		template <typename T>
		void MomServer<T>::on_request(session_t* session, cbuf_t* pcb, req_cb_t cb) {
			error_no_t me;

			do {
				route_t route;
				if (!pcb->template get<route_t>(route)) {
					me = ME_ReadRoute;
					break;
				}

				// < 0 => internal ops
				if (route.ops < 0) {
					switch (static_cast<MomOps>(route.ops)) {
						case MomOps::Register2Gate: {
							node_register_t ret;
							if (!pcb->template get<node_register_t>(ret, sizeof(route_t))) {
								me = ME_ReadRegisterInfo;
							}
							else {
								if (m_nodes.find(ret.nid) != m_nodes.end()) {
									me = ME_NodeIdAlreadyExist;
								}
								else {
									m_nodes[ret.nid] = std::make_shared<node_info_t>(ret, session->get_id());
									me = Success;
								}
							}
							break;
						}

						case MomOps::Coordinate: {
							coordinate_req_t ret;
							if (!pcb->template get<coordinate_req_t>(ret, sizeof(route_t))) {
								me = ME_ReadCoordinateInfo;
							}
							else {
								// 获取coordination 会话
								auto tarSession = get_session_by_ntype(NT_Coordination);
								if (tarSession == nullptr) {
									// target node not exist
									me = ME_CoordinationServerNotExist;
									break;
								}

								// forward
								tarSession->request(pcb, [cb](error_no_t success, cbuf_t* pcb) {
									                    cb(success, pcb);
								                    });

								return;
							}
							break;
						}


						case MomOps::Register2Coordination: {
							node_register_t ret;
							if (!pcb->template get<node_register_t>(ret, sizeof(route_t))) {
								me = ME_ReadRegisterInfo;
							}
							else {
								// 获取coordination 会话
								auto tarSession = get_session_by_ntype(NT_Coordination);
								if (tarSession == nullptr) {
									// target node not exist
									me = ME_CoordinationServerNotExist;
									break;
								}

								// forward
								tarSession->request(pcb, [cb](error_no_t success, cbuf_t* pcb) {
									                    cb(success, pcb);
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
				auto tarSession = get_session_by_nid(route.nid);
				if (tarSession == nullptr) {
					// target node not exist
					me = ME_TargetNodeNotExist;
					break;
				}

				// forward
				tarSession->request(pcb, [cb](error_no_t success, cbuf_t* pcb) {
					                    cb(success, pcb);
				                    });

				return;
			}
			while (false);

			cb(me, pcb);
		}

		template <typename T>
		void MomServer<T>::on_push(session_t* session, cbuf_t* pcb) {
			do {
				auto route = pcb->template get_head_ptr<route_t>();
				if (route == nullptr) {
					break;
				}

				// get target session
				auto tarSession = get_session_by_nid(route->nid);
				if (tarSession == nullptr) {
					// target node not exist
					break;
				}

				// forward
				tarSession->push(pcb, nullptr);
				return;
			}
			while (false);

			RELEASE_CBUF(pcb);
		}

		template <typename T>
		void MomServer<T>::on_open(bool success, session_t* session) { }

		template <typename T>
		void MomServer<T>::on_close(session_t* session) {
			auto it = m_nodes.find(session->get_id());
			if (it != m_nodes.end()) {
				if (it->second->reg.ntype != NT_Coordination) {
					// 若非协调服务，则要通知协调服改服务不可用
					auto tarSession = get_session_by_ntype(NT_Coordination);
					if (tarSession != nullptr) {
						auto pcb = NEW_CBUF();
						pcb->reset();
						auto route = pcb->template pre_write_head<route_t>();
						if (!route) {
							RELEASE_CBUF(pcb);
						}
						else {
							route->ops = MomOps::UnregisterFromCoordination;
							auto req = pcb->template pre_write<node_unregister_t>();
							if (!req) {
								RELEASE_CBUF(pcb);
							}
							else {
								req->nid = it->second->reg.nid;
								tarSession->push(pcb, nullptr);
							}
						}
					}
				}

				m_nodes.erase(it);
			}
		}
	}
}
