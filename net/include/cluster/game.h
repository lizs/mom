// lizs 2017.3.2
#pragma once
#include "node.h"

namespace VK {
	namespace Cluster {
		using namespace Net;
		// 协调服务器
		// 负责为客户端分配恰当的游戏服务器		
		class NET_API Game : public Node {
		public:
			explicit Game(const node_config_t& cfg)
				: Node(cfg) {}

			~Game() override;
		protected:
			void on_ready(std::shared_ptr<client_t> mc) override;

			void on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb, req_cb_t cb) override;
			void on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb) override;
		private:
			bool m_registering2Coordination = false;
		};
	}
}
