#include<functional>
#include "tcp_server.h"
#include "tcp_client.h"
#include <session.h>

using namespace Bull;
char reqData[1024] = "I'm a request";
char repData[1024] = "I'm a response";
char pushData[1024] = "I'm a push";
const char* default_ip = "127.0.0.1";
using session_t = Session<128>;
using Client = TcpClient<session_t>;
using Server = TcpServer<session_t>;
Client* client;
Server* server;

void push() {
	client->push(pushData, strlen(pushData) + 1, [](bool success) {
		             if (success)
			             push();
	             });
}

void push(session_t * session) {
	session->push(pushData, strlen(pushData) + 1, [](bool success) {});
}

void request() {
	client->request(reqData, strlen(reqData) + 1, [](bool success, Client::cbuf_t* pcb) {
		                if (success) {
			                request();
		                }
	                });
}

int main(int argc, char** argv) {
	if (argc > 1) {
		printf("%s %s\n", argv[0], argv[1]);

		if (strcmp(argv[1], "s") == 0) {
			server = new Server("0.0.0.0", 5001,
			                    // request handler
			                    [](Server::session_t * session, const char* data, cbuf_len_t len, char** responseData, cbuf_len_t& responseLen) {
				                    // response
				                    *responseData = repData;
				                    responseLen = strlen(repData) + 1;

				                    // true=>target request has been responsed correctly
				                    return true;
			                    },

			                    // push handler
			                    [](Server::session_t * session, const char* data, cbuf_len_t len) {
				                    // for example
				                    // do nothing here
				                    push(session);
			                    });
			server->startup();
			RUN_UV_DEFAULT_LOOP();
			server->shutdown();
			delete server;
		}
		else if (strcmp(argv[1], "c") == 0) {
			client = new Client(argc > 2 ? argv[2] : default_ip, 5001,
			                    // session established callback
			                    [](bool success, Client::session_t* session) {
				                    if (success) {
					                    push(session);
				                    }
			                    },

			                    // session closed callback
			                    [](Client::session_t* session) { },

			                    // request handler
			                    [](Server::session_t* session, const char* data, cbuf_len_t len, char** responseData, cbuf_len_t& responseLen) {
				                    return false;
			                    },
			                    // push handler
			                    [](Server::session_t* session, const char* data, cbuf_len_t len) {
				                    push(session);
			                    },
			                    true);
			client->startup();
			RUN_UV_DEFAULT_LOOP();
			client->shutdown();
			delete client;
		}
		return 0;
	}

	return -1;
}
