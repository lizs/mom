// lizs 2017.2.28
#pragma once
#include <vector>
#include <memory>
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
		class NET_API Node {
		public:
			typedef MomClient<Node> client_t;
			friend class client_t;

			virtual ~Node() {}

			explicit Node(const node_config_t& cfg);

			bool startup();
			bool shutdown();

		protected:
			virtual void on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb, req_cb_t cb);
			virtual void on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb);
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
	}
}
