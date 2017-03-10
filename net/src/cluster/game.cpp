#include "cluster/game.h"
#include "session.h"

namespace VK {
	namespace Cluster {
		using namespace Net;
		Game::~Game() {}

		void Game::on_ready(std::shared_ptr<client_t> mc) {
			if (m_registering2Coordination)
				return;

			mc->reg_2_coordination();
			m_registering2Coordination = true;
		}

		void Game::on_request(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb, req_cb_t cb) {
			pcb->reset();
			cb(Success, pcb);
		}

		void Game::on_push(std::shared_ptr<client_t> mc, session_t* session, cbuf_ptr_t pcb) {
		}
	}
}
