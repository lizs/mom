// lizs 2017.2.27
#pragma once
#include "tcp_client.h"
#include "proto.h"

namespace VK {
	namespace Cluster {
		using namespace Net;
		template <typename TSession, typename THost>
		class MomClient : public std::enable_shared_from_this<MomClient<TSession, THost>> {
		public:
			TYPE_DEFINES(TSession)
			virtual ~MomClient() {}
			explicit MomClient(THost* host, const char* ip, int port, node_id_t nid, node_type_t ntype);

			bool startup();
			bool shutdown();

			void request(cbuf_t* pcb, req_cb_t cb);
			void push(cbuf_t* pcb);

			void reg_2_coordination();

		private:
			void on_push(session_t* session, cbuf_t* pcb);
			void on_request(session_t* session, cbuf_t* pcb, req_cb_t cb);
			void on_open(bool success, session_t* session);
			void on_close(session_t* session);
			void reg_2_gate();
			void re_reg_2_gate();
			void re_reg_2_coordination();

			node_id_t m_id;
			node_type_t m_type;
			TcpClient<TSession> m_tcpClient;
			Scheduler m_scheduler;
			THost* m_host;
			const timer_period_t RetryDelay = 2 * 1000;
		};

		template <typename TSession, typename THost>
		MomClient<TSession, THost>::MomClient(THost* host, const char* ip, int port,
		                                      node_id_t nid, node_type_t ntype) :
			m_host(host),
			m_id(nid),
			m_type(ntype),
			m_tcpClient(ip, port,
			            std::bind(&MomClient::on_open, this, std::placeholders::_1, std::placeholders::_2),
			            std::bind(&MomClient::on_close, this, std::placeholders::_1),
			            std::bind(&MomClient::on_request, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			            std::bind(&MomClient::on_push, this, std::placeholders::_1, std::placeholders::_2)) {}

		template <typename TSession, typename THost>
		bool MomClient<TSession, THost>::startup() {
			return m_tcpClient.startup();
		}

		template <typename TSession, typename THost>
		bool MomClient<TSession, THost>::shutdown() {
			return m_tcpClient.shutdown();
		}

		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::re_reg_2_gate() {
			m_scheduler.invoke(RetryDelay, [this]() {
				                   reg_2_gate();
			                   });
		}

		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::reg_2_gate() {
			auto pcb = NEW_CBUF();
			pcb->reset();
			auto route = pcb->template pre_write_head<route_t>();
			route->ops = MomOps::Register2Gate;

			auto reg = pcb->template pre_write<node_register_t>();
			if (!reg) {
				LOG("Register gate not triggled");
				RELEASE_CBUF(pcb);
				re_reg_2_gate();
				return;
			}
			reg->nid = m_id;
			reg->ntype = m_type;

			// register
			m_tcpClient.request(pcb, [this](error_no_t en, cbuf_t* pcb) {
				                    if (en != Success) {
					                    re_reg_2_gate();
				                    }
				                    else {
					                    m_host->on_ready(this->shared_from_this());
				                    }
				                    LOG("Register gate result : %s", mom_str_err(en));
			                    });
		}

		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::reg_2_coordination() {
			auto pcb = NEW_CBUF();
			pcb->reset();
			auto route = pcb->template pre_write_head<route_t>();
			route->ops = MomOps::Register2Coordination;

			auto reg = pcb->template pre_write<node_register_t>();
			if (!reg) {
				LOG("Register coordination not triggled");
				RELEASE_CBUF(pcb);
				re_reg_2_gate();
				return;
			}
			reg->nid = m_id;
			reg->ntype = m_type;

			// register
			m_tcpClient.request(pcb, [this](error_no_t en, cbuf_t* pcb) {
				                    if (en != Success) {
					                    re_reg_2_coordination();
				                    }
				                    LOG("Register coordination result : %s", mom_str_err(en));
			                    });
		}

		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::re_reg_2_coordination() {
			m_scheduler.invoke(RetryDelay, [this]() {
				                   reg_2_coordination();
			                   });
		}

		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::on_open(bool success, session_t* session) {
			if (!success) return;
			reg_2_gate();
		}

		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::on_close(session_t* session) {
			m_host->on_down(this->shared_from_this());
		}

		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::on_request(session_t* session, cbuf_t* pcb, req_cb_t cb) {
			m_host->on_request(this->shared_from_this(), session, pcb, cb);
		}

		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::on_push(session_t* session, cbuf_t* pcb) {
			m_host->on_push(this->shared_from_this(), session, pcb);
		}


		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::request(cbuf_t* pcb, req_cb_t cb) {
			m_tcpClient.request(pcb, cb);
		}

		template <typename TSession, typename THost>
		void MomClient<TSession, THost>::push(cbuf_t* pcb) {
			m_tcpClient.push(pcb, nullptr);
		}
	}
}

