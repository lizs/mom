// lizs 2017.3.2
#pragma once
#include <memory>
#include <map>

namespace VK {
	namespace Cluster {
		using namespace Net;

		// 协调服务器
		// 负责为客户端分配恰当的游戏服务器		
		class NET_API Coordination : public Node {
		public:
			explicit Coordination(const node_config_t& cfg)
				: Node(cfg) {}

			~Coordination() override;
		protected:
			void on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb, req_cb_t cb) override;
			void on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb) override;

		private:
			bool get_next_candidate(node_type_t ntype, node_id_t& nid);
			void remove_candidate(node_id_t nid);
			error_no_t insert_candidata(node_register_t reg, session_id_t sid);

			std::map<node_id_t, std::shared_ptr<node_info_t>> m_nodes;
			std::map<node_type_t, std::vector<node_id_t>> m_categories;
		};
	}
}
