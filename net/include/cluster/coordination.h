// lizs 2017.3.2
#pragma once
#include <memory>
#include <map>
#include "node.h"
#include "proto.h"
#include "../net.h"

namespace VK {
	namespace Cluster {
		using namespace Net;
		// 协调服务器
		// 负责为客户端分配恰当的游戏服务器
		template <typename T>
		class Coordination : public Node<T> {
			typedef Node<T> Base;
			TYPE_DEFINES(T)
		public:
			explicit Coordination(const node_config_t& cfg)
				: Node<T>(cfg) {}

			~Coordination() override;
		protected:
			void on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb, req_cb_t cb) override;
			void on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb) override;

		private:
			bool get_next_candidate(node_type_t ntype, node_id_t& nid);
			void remove_candidate(node_id_t nid);
			error_no_t insert_candidata(node_register_t reg, session_id_t sid);

			std::map<node_id_t, std::shared_ptr<node_info_t>> m_nodes;
			std::map<node_type_t, std::vector<node_id_t>> m_categories;
		};

		template <typename T>
		Coordination<T>::~Coordination() {}

		template <typename T>
		bool Coordination<T>::get_next_candidate(node_type_t ntype, node_id_t& nid) {
			auto it = m_categories.find(ntype);
			if (it == m_categories.end())
				return false;

			if (it->second.empty()) return false;
			auto index = rand() % it->second.size();
			nid = it->second[index];
			return true;
		}

		template <typename T>
		void Coordination<T>::remove_candidate(node_id_t nid) {
			auto it = m_nodes.find(nid);
			node_type_t ntype;
			if(it != m_nodes.end()) {
				ntype = it->second->reg.ntype;
				m_nodes.erase(it);
			}

			auto it1 = m_categories.find(ntype);
			if(it1 == m_categories.end()) return;

			auto & cadinates = it1->second;
			auto it2 = std::find(cadinates.begin(), cadinates.end(), nid);
			if(it2 == cadinates.end()) return;
			cadinates.erase(it2);
		}

		template <typename T>
		error_no_t Coordination<T>::insert_candidata(node_register_t reg, session_id_t sid) {
			if (m_nodes.find(reg.nid) != m_nodes.end()) {
				return ME_NodeIdAlreadyExist;
			}

			m_nodes[reg.nid] = std::make_shared<node_info_t>(reg, sid);
			
			auto it = m_categories.find(reg.ntype);
			if (it == m_categories.end()) {
				m_categories[reg.ntype] = { reg.nid };
			}
			else {
				auto & cadinates = it->second;
				if(std::find(cadinates.begin(), cadinates.end(), reg.nid) == cadinates.end()) {
					cadinates.push_back(reg.nid);
				}
			}

			return Success;
		}

		template <typename T>
		void Coordination<T>::on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb, req_cb_t cb) {
			error_no_t me;

			do {
				route_t route;
				if (!pcb->template get<route_t>(route)) {
					me = ME_ReadRoute;
					break;
				}

				switch (static_cast<MomOps>(route.ops)) {
					case MomOps::Coordinate: {
						// 请求协调	
						coordinate_req_t ret;
						if (!pcb->template get<coordinate_req_t>(ret, sizeof(route_t))) {
							me = ME_InvlidOps;
						}
						else {
							node_id_t nid;
							if (!get_next_candidate(ret.ntype, nid)) {
								me = ME_TargetServerNotExist;
							}

							// response
							pcb->set_tail(pcb->get_head() + sizeof(route_t));
							auto rep = pcb->template pre_write<coordinate_rep_t>();
							rep->nid = nid;
							me = Success;
						}
						break;
					}

					case MomOps::Register2Coordination: {
						node_register_t ret;
						if (!pcb->template get<node_register_t>(ret, sizeof(route_t))) {
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

		template <typename T>
		void Coordination<T>::on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb) {
			error_no_t me;

			do {
				route_t route;
				if (!pcb->template get<route_t>(route)) {
					me = ME_ReadRoute;
					break;
				}

				switch (static_cast<MomOps>(route.ops)) {
					case MomOps::UnregisterFromCoordination: {
						node_unregister_t ret;
						if (!pcb->template get<node_unregister_t>(ret, sizeof(route_t))) {
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
			}
			while (false);

			RELEASE_CBUF(pcb);
			if (me != Success) {
				LOG(mom_str_err(me));
			}
		}
	}
}
