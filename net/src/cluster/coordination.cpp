#include "cluster/node.h"
#include "cluster/coordination.h"
#include "session.h"

namespace VK {
	namespace Cluster {

		Coordination::~Coordination() {}

		bool Coordination::get_next_candidate(node_type_t ntype, node_id_t& nid) {
			auto it = m_categories.find(ntype);
			if (it == m_categories.end())
				return false;

			if (it->second.empty()) return false;
			auto index = rand() % it->second.size();
			nid = it->second[index];
			return true;
		}

		void Coordination::remove_candidate(node_id_t nid) {
			auto it = m_nodes.find(nid);
			node_type_t ntype;
			if (it != m_nodes.end()) {
				ntype = it->second->reg.ntype;
				m_nodes.erase(it);
			}

			auto it1 = m_categories.find(ntype);
			if (it1 == m_categories.end()) return;

			auto& cadinates = it1->second;
			auto it2 = find(cadinates.begin(), cadinates.end(), nid);
			if (it2 == cadinates.end()) return;
			cadinates.erase(it2);
		}

		error_no_t Coordination::insert_candidata(node_register_t reg, session_id_t sid) {
			if (m_nodes.find(reg.nid) != m_nodes.end()) {
				return ME_NodeIdAlreadyExist;
			}

			m_nodes[reg.nid] = std::make_shared<node_info_t>(reg, sid);

			auto it = m_categories.find(reg.ntype);
			if (it == m_categories.end()) {
				m_categories[reg.ntype] = {reg.nid};
			}
			else {
				auto& cadinates = it->second;
				if (find(cadinates.begin(), cadinates.end(), reg.nid) == cadinates.end()) {
					cadinates.push_back(reg.nid);
				}
			}

			return Success;
		}

		void Coordination::on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb, req_cb_t cb) {
			error_no_t me;

			do {
				req_header_t route;
				if (!pcb->get<req_header_t>(route)) {
					me = ME_ReadHeader;
					break;
				}

				switch (static_cast<MomOps>(route.ops)) {
					case Coordinate: {
						// ÇëÇóÐ­µ÷	
						coordinate_req_t ret;
						if (!pcb->get<coordinate_req_t>(ret, sizeof(req_header_t))) {
							me = ME_InvlidOps;
						}
						else {
							node_id_t nid;
							if (!get_next_candidate(ret.ntype, nid)) {
								me = ME_TargetServerNotExist;
							}

							// response
							pcb->reset<coordinate_rep_t>();
							auto rep = pcb->pre_write<coordinate_rep_t>();
							rep->nid = nid;
							me = Success;
						}
						break;
					}

					case Register2Coordination: {
						node_register_t ret;
						if (!pcb->get<node_register_t>(ret, sizeof(req_header_t))) {
							me = ME_InvlidOps;
						}
						else {
							me = insert_candidata(ret, session->get_id());
						}
						break;
					}

					default:
						me = ME_InvlidOps;
						break;
				}
			}
			while (false);

			cb(me, pcb);
		}

		void Coordination::on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb) {
			error_no_t me;

			push_header_t route;
			if (!pcb->get<push_header_t>(route)) {
				me = ME_ReadHeader;
				return;
			}

			switch (static_cast<MomOps>(route.ops)) {
				case UnregisterFromCoordination: {
					node_unregister_t ret;
					if (!pcb->get<node_unregister_t>(ret, sizeof(push_header_t))) {
						me = ME_ReadUnregisterInfo;
					}
					else {
						remove_candidate(ret.nid);
						me = Success;
					}
					break;
				}

				default:
					me = ME_InvlidOps;
					break;
			}

			if (me != Success) {
				LOG(mom_str_err(me));
			}
		}
	}
}
