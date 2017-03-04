// lizs 2017.3.2
#pragma once
#include "node.h"

namespace VK {
	namespace Cluster {
		using namespace Net;
		// 协调服务器
		// 负责为客户端分配恰当的游戏服务器
		template <typename T>
		class Game : public Node<T> {
			typedef Node<T> Base;
			TYPE_DEFINES(T)
		public:
			explicit Game(const node_config_t& cfg)
				: Node<T>(cfg) {}

			~Game() override;
		protected:
			void on_ready(std::shared_ptr<client_t> mc) override;

			void on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb, req_cb_t cb) override;
			void on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb) override;
		private:
			bool m_registering2Coordination = false;
		};

		template <typename T>
		Game<T>::~Game() {}

		template <typename T>
		void Game<T>::on_ready(std::shared_ptr<client_t> mc) {
			if(m_registering2Coordination)
				return;

			mc->reg_2_coordination();
			m_registering2Coordination = true;
		}

		template <typename T>
		void Game<T>::on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb, req_cb_t cb) {
			pcb->reset();
			cb(Success, pcb);
		}

		template <typename T>
		void Game<T>::on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_t* pcb) {
			RELEASE_CBUF(pcb);
		}
	}
}
