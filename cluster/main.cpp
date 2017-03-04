#include <net.h>
#include <session.h>
#include <monitor.h>
#include <cluster/gate.h>
#include <cluster/game.h>
#include <cluster/coordination.h>
#include <iostream>

using namespace VK;
using namespace Net;
using namespace Cluster;

const char* default_ip = "127.0.0.1";
const int GATE_1_PORT = 5001;
const int GATE_2_PORT = 5002;
const node_id_t GATE_1_ID = 1;
const node_id_t GATE_2_ID = 2;
const node_id_t COORDINATION_ID = 100;
const node_id_t GAME_1_ID = 11;
const node_id_t GAME_2_ID = 12;
using session_t = Session<1024>;
using cbuf_t = session_t::cbuf_t;

enum Ops {
	OPS_Request = 1,
	OPS_Push,
};

char reqData[1024] = "Hello, I'm a request.";
char pushData[1024] = "Hello, I'm a push.";

TcpClient<session_t>* client;
Scheduler scheduler;

node_id_t GAME_ID = 0;
bool use_push = false;

void request() {
	auto pcb = NEW_CBUF();
	pcb->reset();

	pcb->write_binary(reqData, strlen(reqData) + 1);
	auto x = pcb->pre_write_head<route_t>();
	x->nid = GAME_ID;
	x->ops = OPS_Request;

	client->request(pcb, [](error_no_t error_no, session_t::cbuf_t* pcb) {
		                RELEASE_CBUF(pcb);
		                if (error_no == Success)
			                request();
		                else {
			                LOG(mom_str_err(error_no));
		                }
	                });
}

void push() {
	auto pcb = NEW_CBUF();
	pcb->reset();

	pcb->write_binary(reqData, strlen(reqData) + 1);
	auto x = pcb->pre_write_head<route_t>();
	x->nid = GAME_ID;
	x->ops = OPS_Push;

	client->push(pcb, [](bool success) {
		             if (success) {
			             push();
			             std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		             }
	             });
}

void coordinate() {
	auto pcb = NEW_CBUF();
	pcb->reset();
	auto head = pcb->pre_write_head<route_t>();
	head->nid = 0;
	head->ops = Coordinate;

	auto req = pcb->pre_write<coordinate_req_t>();
	req->ntype = NT_Game;

	client->request(pcb, [](error_no_t error_no, session_t::cbuf_t* pcb) {
		                if (error_no == Success) {
			                coordinate_rep_t rep;
			                pcb->get(rep, sizeof(route_t));
			                GAME_ID = rep.nid;
			                if (use_push)
				                push();
			                else
				                request();
		                }
		                else {
			                LOG(mom_str_err(error_no));

			                scheduler.invoke(2000, coordinate);
		                }

		                RELEASE_CBUF(pcb);
	                });
}

void run_client(const char* ip, int port, bool up) {
	use_push = up;
	client = new TcpClient<session_t>(ip, port,
	                                  // session_t established callback
	                                  [=](bool success, session_t* session_t) {
		                                  if (success) {
			                                  coordinate();
		                                  }
	                                  },

	                                  // session_t closed callback
	                                  [](session_t* session_t) {},

	                                  // request handler
	                                  [](session_t* session_t, cbuf_t* pcb, session_t::req_cb_t cb) {
		                                  return false;
	                                  },
	                                  // push handler
	                                  [](session_t* session_t, cbuf_t* pcb) { },
	                                  true
	);

	client->startup();
	RUN_UV_DEFAULT_LOOP();
	client->shutdown();

	delete client;
}

void run_coordination() {
	node_config_t cfg;
	cfg.nid = COORDINATION_ID;
	cfg.ntype = NT_Coordination;
	cfg.gate_addresses = {
		{default_ip, GATE_1_PORT},
		{default_ip, GATE_2_PORT},
	};

	auto game = std::make_unique<Coordination<session_t>>(cfg);
	game->startup();
	RUN_UV_DEFAULT_LOOP();
	game->shutdown();
}

void run_game(node_id_t nid) {
	node_config_t cfg;
	cfg.nid = nid;
	cfg.ntype = NT_Game;
	cfg.gate_addresses = {
		{default_ip, GATE_1_PORT},
		{default_ip, GATE_2_PORT},
	};

	auto game = std::make_unique<Game<session_t>>(cfg);
	game->startup();
	RUN_UV_DEFAULT_LOOP();
	game->shutdown();
}

void run_gate(node_id_t nid, const char* ip, int port) {
	auto gate = std::make_unique<Gate<session_t>>(nid, ip, port);

	gate->startup();
	RUN_UV_DEFAULT_LOOP();
	gate->shutdown();
}

int main(int argc, char** argv) {
	if (argc > 1) {
		printf("%s %s\n", argv[0], argv[1]);

		do {
			if (strcmp(argv[1], "gate1") == 0) {
				run_gate(GATE_1_ID, default_ip, GATE_1_PORT);
			}
			else if (strcmp(argv[1], "gate2") == 0) {
				run_gate(GATE_2_ID, default_ip, GATE_2_PORT);
			}
			else if (strcmp(argv[1], "game1") == 0) {
				run_game(GAME_1_ID);
			}
			else if (strcmp(argv[1], "game2") == 0) {
				run_game(GAME_2_ID);
			}
			else if (strcmp(argv[1], "coordination") == 0) {
				run_coordination();
			}
			else if (strcmp(argv[1], "client1") == 0) {
				run_client(default_ip, GATE_1_PORT, true);
			}
			else if (strcmp(argv[1], "client2") == 0) {
				run_client(default_ip, GATE_2_PORT, true);
			}
			else if (strcmp(argv[1], "client3") == 0) {
				run_client(default_ip, GATE_1_PORT, false);
			}
			else if (strcmp(argv[1], "client4") == 0) {
				run_client(default_ip, GATE_2_PORT, false);
			}
		}
		while (false);

		Singleton<MemoryPool<session_t::cbuf_t>>::destroy();
		Singleton<Monitor>::destroy();
		return 0;
	}

	return -1;
}
