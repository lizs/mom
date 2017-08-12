// lizs 2017.6.16
#pragma once
#include "defines.h"
#include "session.h"
#include "util/mgr.h"

namespace VK {
	namespace Net {
		// subjects manager
		class SubMgr : public Mgr<std::string, Mgr<session_id_t, session_wk_ptr_t>> {
			typedef Mgr<std::string, Mgr<session_id_t, session_wk_ptr_t>> Base;
		public:
			void add(const std::string & sub, session_ptr_t session) {
				(*this)[sub][session->get_id()] = session;
			}

			void remove(const std::string & sub) {
				Base::remove(sub);
			}

			void remove(const std::string & sub, session_id_t sid) {
				(*this)[sub].remove(sid);
			}

			void remove(const std::string & sub, session_ptr_t session) {
				remove(sub, session->get_id());
			}

			void remove(session_ptr_t session) {
				for (auto & it : *this) {
					it.second.remove(session->get_id());
				}
			}

			void pub(const std::string & sub, cbuf_ptr_t pcb) {
				auto sessions = std::move((*this)[sub].values());
				if (sessions.empty())
					return;

				multicast(pcb, sessions);
			}

		private:
#pragma region ("×é²¥")
			void multicast(cbuf_ptr_t pcb, std::vector<session_wk_ptr_t>& sessions) const {
				auto pcbs = std::move(pack(pcb, Push));
				if (pcbs.size() == 0)
					return;

				for (auto session : sessions) {
					if (session.expired())
						continue;
					session.lock()->send(pcbs, nullptr);
				}
			}
#pragma endregion 
		};
	}
}
