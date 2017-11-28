#pragma once

#include <circular_buf.h>
#include <session.h>
#include <monitor.h>
#include <tcp_client.h>
#include <tcp_server.h>

using namespace VK;
using namespace Net;

char reqBytes[] = "Hello req!";
char pushBytes[] = "Hello push!";
char pubBytes[] = "Hello PUB/SUB!";

class ServerHandler : public VK::Net::IHandler {
	void on_connected(bool success, session_ptr_t session) override { }

	void on_closed(session_ptr_t session) override { }

	void on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) override {
		if (cb) {
			// echo back
			cb(Success, pcb);
		}
	}

	error_no_t on_push(session_ptr_t session, cbuf_ptr_t pcb) override {
		//Logger::instance().debug(pcb->get_head_ptr());
		return Success;
	}
};

static void run_server() {
	auto server = std::make_shared<TcpServer>("127.0.0.1", 5002, std::make_shared<ServerHandler>());
	server->startup();

	RUN_UV_DEFAULT_LOOP();
	server->shutdown();
}

class ClientHandler : public VK::Net::IHandler {
	VK::Net::Scheduler scheduler;

	void on_connected(bool success, session_ptr_t session) override {
		if (success) {
			session->sub("Channel");
			push(session);
			//pub(session);
		}
	}

	void on_closed(session_ptr_t session) override { }

	void on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) override {
		cb(0, nullptr);
	}

	error_no_t on_push(session_ptr_t session, cbuf_ptr_t pcb) override {
		return Success;
	}

	void request(session_ptr_t session) {
		auto pcb = alloc_cbuf(cbuf_len_t(strlen(reqBytes) + 1));
		pcb->write_binary(reqBytes, cbuf_len_t(strlen(reqBytes) + 1));

		session->request(pcb, [this](session_ptr_t session, error_no_t err, cbuf_ptr_t pcb) {
			                 if (!err)
				                 request(session);
		                 });
	}

	void pub(session_ptr_t session) {
		auto pcb = alloc_cbuf(cbuf_len_t(strlen(pubBytes) + 1));
		pcb->write_binary(pubBytes, cbuf_len_t(strlen(pubBytes) + 1));
		session->pub("Channel", pcb);

		scheduler.invoke(1, [this, session](any _) {
			                 this->pub(session);
		                 });
	}

	void push(session_ptr_t session) {
		auto pcb = alloc_cbuf(cbuf_len_t(strlen(pushBytes) + 1));
		pcb->write_binary(pushBytes, cbuf_len_t(strlen(pushBytes) + 1));
		session->push(pcb, nullptr);

		scheduler.invoke(1, [this, session](any _) {
			this->push(session);
		});
	}
};

static void run_client_test(int cnt) {
	std::vector<std::shared_ptr<TcpClient>> clients;
	for (auto i = 0; i < cnt; ++i) {
		clients.push_back(std::make_shared<TcpClient>("127.0.0.1", 5002, std::make_shared<ClientHandler>()));
	}

	for (auto client : clients) {
		client->startup();
	}

	RUN_UV_DEFAULT_LOOP();

	for (auto client : clients) {
		client->shutdown();
	}
}
