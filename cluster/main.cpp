#include <defines.h>
#include <session.h>
#include <monitor.h>
#include <cluster/gate.h>
#include <cluster/game.h>
#include <cluster/coordination.h>
#include <iostream>
#include <thread>
#include <scheduler.h>

using namespace VK;
using namespace Net;
using namespace Cluster;

const char* default_ip = "127.0.0.1";
const char* default_host = "localhost";
const int GATE_1_PORT = 5001;
const int GATE_2_PORT = 5002;
const node_id_t GATE_1_ID = 1;
const node_id_t GATE_2_ID = 2;
const node_id_t COORDINATION_ID = 100;
const node_id_t GAME_1_ID = 11;
const node_id_t GAME_2_ID = 12;

enum Ops {
	OPS_Request = 1,
	OPS_Push,
};

char reqData[1024] = "Hello, I'm a request.";
char pushData[1024] = "Hello, I'm a push.";
char broadcastData[1024] = "Hello, I'm a broadcast.";

TcpClient* client;
Scheduler scheduler;

node_id_t GAME_ID = 0;

enum Mode {
	M_Request,
	M_Push,
	M_Broadcast,
	M_Raw_Push,
	M_Raw_Request,
};

Mode g_mode = M_Request;

void raw_push() {
	auto pcb = alloc_cbuf(cbuf_len_t(strlen(pushData) + 1));
	pcb->write_binary(pushData, cbuf_len_t(strlen(pushData) + 1));

	client->push(pcb, [](bool success) {
		             if (success) {
			             raw_push();
			             //std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		             }
	             });
}

void raw_request() {
	auto pcb = alloc_cbuf(cbuf_len_t(strlen(reqData) + 1));
	pcb->write_binary(reqData, cbuf_len_t(strlen(reqData) + 1));

	client->request(pcb, [](error_no_t error_no, cbuf_ptr_t pcb) {
		                raw_request();
		                //if (error_no == Success)
		                //	raw_request();
		                //else {
		                //	LOG(mom_str_err(error_no));
		                //}
	                });
}

void request() {
	auto pcb = alloc_request(OPS_Request, GAME_ID, cbuf_len_t(strlen(reqData) + 1));
	pcb->write_binary(reqData, cbuf_len_t(strlen(reqData) + 1));

	client->request(pcb, [](error_no_t error_no, cbuf_ptr_t pcb) {
		                if (error_no == Success)
			                request();
		                else {
			                LOG(mom_str_err(error_no));
		                }
	                });
}

void push() {
	auto pcb = alloc_push(OPS_Push, GAME_ID, cbuf_len_t(strlen(pushData) + 1));
	pcb->write_binary(pushData, cbuf_len_t(strlen(pushData) + 1));

	client->push(pcb, [](bool success) {
		             if (success) {
			             push();
			             std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		             }
	             });
}

void broadcast() {
	auto pcb = alloc_broadcast(cbuf_len_t(strlen(broadcastData) + 1));
	pcb->write_binary(broadcastData, cbuf_len_t(strlen(broadcastData) + 1));

	client->push(pcb, [](bool success) {
		             if (success) {
			             broadcast();
			             std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		             }
	             });
}

void coordinate() {
	auto pcb = alloc_request<coordinate_req_t>(Coordinate, 0);
	auto req = pcb->pre_write<coordinate_req_t>();
	req->ntype = NT_Game;

	client->request(pcb, [](error_no_t error_no, cbuf_ptr_t pcb) {
		                if (error_no == Success) {
			                coordinate_rep_t rep;
			                pcb->get(rep);
			                GAME_ID = rep.nid;
			                switch (g_mode) {
				                case M_Request:
					                request();
					                break;
				                case M_Push:
					                push();
					                break;
				                case M_Broadcast:
					                broadcast();
					                break;
				                default:
					                break;
			                }
		                }
		                else {
			                LOG(mom_str_err(error_no));
			                scheduler.invoke(2000, coordinate);
		                }
	                });
}

void run_raw_server(const char* ip, int port) {
	auto game = std::make_unique<TcpServer>(ip, port, nullptr, nullptr, nullptr, [](session_t* session, cbuf_ptr_t pcb) { });

	game->startup();
	RUN_UV_DEFAULT_LOOP();
	game->shutdown();
}

void run_client(const char* ip, int port, Mode mode) {
	g_mode = mode;
	client = new TcpClient(ip, port,
	                       // session_t established callback
	                       [=](bool success, session_t* session_t) {
		                       if (success) {
			                       switch (g_mode) {
				                       case M_Request:
				                       case M_Push:
				                       case M_Broadcast:
					                       coordinate();
					                       break;

				                       case M_Raw_Push:
					                       raw_push();
					                       break;
				                       case M_Raw_Request:
					                       raw_request();
					                       break;
				                       default:
					                       break;
			                       }
		                       }
	                       },

	                       // session_t closed callback
	                       [](session_t* session_t) {},

	                       // request handler
	                       [](session_t* session_t, cbuf_ptr_t pcb, req_cb_t cb) {
		                       return false;
	                       },
	                       // push handler
	                       [](session_t* session_t, cbuf_ptr_t pcb) { },
	                       true);

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
		{default_host, GATE_1_PORT},
		{default_host, GATE_2_PORT},
	};

	auto game = std::make_unique<Coordination>(cfg);
	game->startup();
	RUN_UV_DEFAULT_LOOP();
	game->shutdown();
}

void run_game(node_id_t nid) {
	node_config_t cfg;
	cfg.nid = nid;
	cfg.ntype = NT_Game;
	cfg.gate_addresses = {
		{default_host, GATE_1_PORT},
		{default_host, GATE_2_PORT},
	};

	auto game = std::make_unique<Game>(cfg);
	game->startup();
	RUN_UV_DEFAULT_LOOP();
	game->shutdown();
}

void run_gate(node_id_t nid, const char* ip, int port) {
	auto gate = std::make_shared<Gate>(nid, ip, port);

	gate->startup();
	RUN_UV_DEFAULT_LOOP();
	gate->shutdown();
}

int main(int argc, char** argv) {
	if (argc > 1) {
		printf("%s %s\n", argv[0], argv[1]);

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
			run_client(default_host, GATE_1_PORT, M_Push);
		}
		else if (strcmp(argv[1], "client2") == 0) {
			run_client(default_host, GATE_2_PORT, M_Push);
		}
		else if (strcmp(argv[1], "client3") == 0) {
			run_client(default_host, GATE_1_PORT, M_Request);
		}
		else if (strcmp(argv[1], "client4") == 0) {
			run_client(default_host, GATE_2_PORT, M_Request);
		}
		else if (strcmp(argv[1], "client5") == 0) {
			run_client(default_host, GATE_2_PORT, M_Broadcast);
		}
		else if (strcmp(argv[1], "client") == 0) {
			run_client(default_host, GATE_2_PORT, M_Raw_Push);
		}
		else if (strcmp(argv[1], "server") == 0) {
			run_raw_server(default_ip, GATE_2_PORT);
		}

		return 0;
	}

	return -1;
}
