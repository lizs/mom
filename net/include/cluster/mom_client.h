// lizs 2017.2.27
#pragma once
#include "tcp_client.h"
#include <util.h>

namespace VK {
	namespace Cluster {
		using namespace Net;
		template <typename TParent>
		class MomClient : public std::enable_shared_from_this<MomClient<TParent>> {
		public:
			virtual ~MomClient() {}
			explicit MomClient(TParent* host, const char* ip, int port, node_id_t nid, node_type_t ntype);

			bool startup() const;
			bool shutdown() const;

			void request(cbuf_ptr_t pcb, req_cb_t cb) const;
			void push(cbuf_ptr_t pcb) const;

			void reg_2_coordination();

		private:
			void on_push(session_t* session, cbuf_ptr_t pcb);
			void on_request(session_t* session, cbuf_ptr_t pcb, req_cb_t cb);
			void on_open(bool success, session_t* session);
			void on_close(session_t* session);
			void reg_2_gate();
			void re_reg_2_gate();
			void re_reg_2_coordination();

			node_id_t m_id;
			node_type_t m_type;
			TcpClient m_tcpClient;
			Scheduler m_scheduler;
			TParent* m_parent;
			const timer_period_t RetryDelay = 2 * 1000;
		};

		template <typename THost>
		MomClient<THost>::MomClient(THost* parent, const char* host, int port,
		                                      node_id_t nid, node_type_t ntype) :
			m_id(nid),
			m_type(ntype),
			m_tcpClient(host, port,
			            std::bind(&MomClient::on_open, this, std::placeholders::_1, std::placeholders::_2),
			            std::bind(&MomClient::on_close, this, std::placeholders::_1),
			            std::bind(&MomClient::on_request, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			            std::bind(&MomClient::on_push, this, std::placeholders::_1, std::placeholders::_2)),
			m_parent(parent) {}

		template <typename THost>
		bool MomClient<THost>::startup() const {
			return m_tcpClient.startup();
		}

		template <typename THost>
		bool MomClient<THost>::shutdown() const {
			return m_tcpClient.shutdown();
		}

		template <typename THost>
		void MomClient<THost>::re_reg_2_gate() {
			m_scheduler.invoke(RetryDelay, [this]() {
				                   reg_2_gate();
			                   });
		}

		template <typename THost>
		void MomClient<THost>::reg_2_gate() {
			auto pcb = alloc_cbuf<node_register_t>();
			auto route = pcb->pre_write_head<req_header_t>();
			route->ops = Register2Gate;

			auto reg = pcb->pre_write<node_register_t>();
			if (!reg) {
				LOG("Register gate not triggled");
				re_reg_2_gate();
				return;
			}
			reg->nid = m_id;
			reg->ntype = m_type;

			// register
			m_tcpClient.request(pcb, [this](error_no_t en, cbuf_ptr_t pcb) {
				                    if (en != Success) {
					                    re_reg_2_gate();
				                    }
				                    else {
					                    m_parent->on_ready(this->shared_from_this());
				                    }
				                    LOG("Register gate result : %s", mom_str_err(en));
			                    });
		}

		template <typename THost>
		void MomClient<THost>::reg_2_coordination() {
			auto pcb = alloc_cbuf<node_register_t>();
			auto route = pcb->pre_write_head<req_header_t>();
			route->ops = Register2Coordination;

			auto reg = pcb->pre_write<node_register_t>();
			if (!reg) {
				LOG("Register coordination not triggled");
				re_reg_2_gate();
				return;
			}
			reg->nid = m_id;
			reg->ntype = m_type;

			// register
			m_tcpClient.request(pcb, [this](error_no_t en, cbuf_ptr_t pcb) {
				                    if (en != Success) {
					                    re_reg_2_coordination();
				                    }
				                    LOG("Register coordination result : %s", mom_str_err(en));
			                    });
		}

		template <typename THost>
		void MomClient<THost>::re_reg_2_coordination() {
			m_scheduler.invoke(RetryDelay, [this]() {
				                   reg_2_coordination();
			                   });
		}

		template <typename THost>
		void MomClient<THost>::on_open(bool success, session_t* session) {
			if (!success) return;
			reg_2_gate();
		}

		template <typename THost>
		void MomClient<THost>::on_close(session_t* session) {
			m_parent->on_down(this->shared_from_this());

			m_scheduler.cancel_all();
		}

		template <typename THost>
		void MomClient<THost>::on_request(session_t* session, cbuf_ptr_t pcb, req_cb_t cb) {
			m_parent->on_request(this->shared_from_this(), session, pcb, cb);
		}

		template <typename THost>
		void MomClient<THost>::on_push(session_t* session, cbuf_ptr_t pcb) {
			m_parent->on_push(this->shared_from_this(), session, pcb);
		}

		template <typename THost>
		void MomClient<THost>::request(cbuf_ptr_t pcb, req_cb_t cb) const {
			m_tcpClient.request(pcb, cb);
		}

		template <typename THost>
		void MomClient<THost>::push(cbuf_ptr_t pcb) const {
			m_tcpClient.push(pcb, nullptr);
		}
	}
}

