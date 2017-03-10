#include "cluster/node.h"
#include "monitor.h"
#include "session.h"

namespace VK {
	namespace Cluster {
		using namespace Net;
		Node::Node(const node_config_t& cfg) : m_id(cfg.nid), m_type(cfg.ntype) {
			for (auto& kv : cfg.gate_addresses) {
				m_clients.push_back(std::make_shared<client_t>(this, kv.first.c_str(), kv.second, m_id, m_type));
			}
		}
	
		bool Node::startup() {
			auto result = true;
			for (auto mc : m_clients) {
				result &= mc->startup();
			}

			return result;
		}
	
		bool Node::shutdown() {
			auto result = true;
			for (auto mc : m_clients) {
				result &= mc->shutdown();
			}

			return result;
		}

		void Node::on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb, req_cb_t cb) {
			pcb->reset();
			cb(ME_NotImplemented, pcb);
		}
	
		void Node::on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb) {
			LOG(mom_str_err(ME_NotImplemented));
		}

		void Node::on_ready(std::shared_ptr<client_t> mc) {
			auto it = find(m_readyClients.begin(), m_readyClients.end(), mc);
			if (it == m_readyClients.end())
				m_readyClients.push_back(mc);
		}
	
		void Node::on_down(std::shared_ptr<client_t> mc) {
			auto it = find(m_readyClients.begin(), m_readyClients.end(), mc);
			if (it != m_readyClients.end())
				m_readyClients.erase(it);
		}
	
		std::shared_ptr<Node::client_t> Node::get_next_mc() {
			if (m_readyClients.empty()) return nullptr;

			++m_mcIndex;
			m_mcIndex %= m_readyClients.size();
			return m_readyClients[m_mcIndex];
		}
	}
}
