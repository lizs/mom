// lizs 2017.2.28
#pragma once
#include <vector>
#include "mom_client.h"

namespace VK {
	namespace Cluster {
		using namespace Net;
		typedef struct {
			node_id_t nid;
			node_type_t ntype;
			std::vector<std::pair<std::string, int>> gate_addresses;
		} node_config_t;

		// 业务服务器抽象
		template <typename T>
		class Node {
		public:
			TYPE_DEFINES(T)
			typedef MomClient<T, Node<T>> client_t;
			friend class client_t;

			virtual ~Node() {}

			explicit Node(const node_config_t& cfg);

			bool startup();
			bool shutdown();

		protected:
			virtual void on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb, req_cb_t cb);
			virtual void on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb);
			virtual void on_ready(std::shared_ptr<client_t> mc);
			virtual void on_down(std::shared_ptr<client_t> mc);

			std::shared_ptr<client_t> get_next_mc();

		private:
			node_id_t m_id;
			node_type_t m_type;

			uint8_t m_mcIndex = 0;
			std::vector<std::shared_ptr<client_t>> m_clients;
			std::vector<std::shared_ptr<client_t>> m_readyClients;
		};

		template <typename T>
		Node<T>::Node(const node_config_t& cfg) : m_id(cfg.nid), m_type(cfg.ntype) {
			for (auto& kv : cfg.gate_addresses) {
				m_clients.push_back(std::make_shared<client_t>(this, kv.first.c_str(), kv.second, m_id, m_type));
			}
		}

		template <typename T>
		bool Node<T>::startup() {
			auto result = true;
			for (auto mc : m_clients) {
				result &= mc->startup();
			}

			return result;
		}

		template <typename T>
		bool Node<T>::shutdown() {
			auto result = true;
			for (auto mc : m_clients) {
				result &= mc->shutdown();
			}

			return result;
		}

		template <typename T>
		void Node<T>::on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb, typename T::req_cb_t cb) {
			pcb->reset();
			cb(ME_NotImplemented, pcb);
		}

		template <typename T>
		void Node<T>::on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb) {
			RELEASE_CBUF(pcb);
		}

		template <typename T>
		void Node<T>::on_ready(std::shared_ptr<client_t> mc) {
			auto it = std::find(m_readyClients.begin(), m_readyClients.end(), mc);
			if (it == m_readyClients.end())
				m_readyClients.push_back(mc);
		}

		template <typename T>
		void Node<T>::on_down(std::shared_ptr<client_t> mc) {
			auto it = std::find(m_readyClients.begin(), m_readyClients.end(), mc);
			if (it != m_readyClients.end())
				m_readyClients.erase(it);
		}

		template <typename T>
		std::shared_ptr<typename Node<T>::client_t> Node<T>::get_next_mc() {
			if (m_readyClients.empty()) return nullptr;

			++m_mcIndex;
			m_mcIndex %= m_readyClients.size();
			return m_readyClients[m_mcIndex];
		}
	}
}
