#pragma once

#include <circular_buf.h>
#include <session.h>
#include <monitor.h>
#include <tcp_client.h>
#include <tcp_server.h>

using namespace VK;
using namespace Net;

char reqBytes[] = "Hello mom!";
char respBytes[] = "Hello IO!";
char pubBytes[] = "Hello PUB/SUB!";

class ServerHandler : public VK::Net::IHandler {
	void on_connected(bool success, session_ptr_t session) override {
	}

	void on_closed(session_ptr_t session) override{
	}

	void on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) override {
		if (cb) {
			auto resp = alloc_cbuf(cbuf_len_t(strlen(respBytes) + 1));
			resp->write_binary(respBytes, cbuf_len_t(strlen(respBytes) + 1));

			cb(Success, resp);
		}
	}

	error_no_t on_push(session_ptr_t session, cbuf_ptr_t pcb) override {
		return Success;
	}
};

static void run_server_test() {
	VK::Net::Scheduler scheduler;

	auto server = std::make_shared<TcpServer>("127.0.0.1", 5002, std::make_shared<ServerHandler>());
	server->startup();

	scheduler.invoke(1000, 1000, [server](any token) {
		auto pcb = alloc_cbuf(cbuf_len_t(strlen(pubBytes) + 1));
		pcb->write_binary(pubBytes, cbuf_len_t(strlen(pubBytes) + 1));
		server->pub("Channel", pcb);
	});

	RUN_UV_DEFAULT_LOOP();
	server->shutdown();
}

class ClientHandler : public VK::Net::IHandler {
	void on_connected(bool success, session_ptr_t session) override {
		if (success)
		{
			session->sub("Channel");
			request(session);
		}
	}

	void on_closed(session_ptr_t session) override {
	}

	void on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) override {
		if (cb) {
			auto resp = alloc_cbuf(cbuf_len_t(strlen(respBytes) + 1));
			resp->write_binary(respBytes, cbuf_len_t(strlen(respBytes) + 1));

			cb(Success, resp);
		}
	}

	error_no_t on_push(session_ptr_t session, cbuf_ptr_t pcb) override {
		Logger::instance().debug(pcb->get_head_ptr());
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
};

static void run_client_test(int cnt) {
	std::vector<std::shared_ptr<TcpClient>> clients;
	for (auto i = 0; i < cnt; ++i) {
		clients.push_back(std::make_shared<TcpClient>("127.0.0.1", 5002, std::make_shared<ClientHandler>(), true, false));
	}

	for (auto client : clients) {
		client->startup();
	}

	RUN_UV_DEFAULT_LOOP();

	for (auto client : clients) {
		client->shutdown();
	}
}
