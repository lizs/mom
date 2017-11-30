#include <session.h>
#include <logger.h>
#include <monitor.h>
#include <tcp_server.h>
#include <iostream>

using namespace VK;
using namespace Net;

class ServerHandler : public VK::Net::IHandler{
	void on_connected(bool success, session_ptr_t session) override {}
	void on_closed(session_ptr_t session) override {}
	void on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) override {
		// echo back
		if(cb) {
			cb(Success, pcb);
		}
	}
	
	error_no_t on_push(session_ptr_t session, cbuf_ptr_t pcb) override {
		// do nothing
		return Success;
	}
};

int main(int argc, char ** argv) {
	VK::Logger::instance().start("mom.log");

	char * host = nullptr;
	int port = 0;

	// extract commands
	for(auto i = 1; i < argc; i+=2){
		if(strcmp(argv[i], "-h") == 0){
			host = argv[i+1];	
		}
		else if(strcmp(argv[i], "-p") == 0){
			port = atoi(argv[i+1]);
		}
		else{
			std::cout << "Example : mom -h localhost -p 5001" << std::endl;
			return -1;
		}
	}
	
	if(host == nullptr){
		std::cout << "host not specified. use '-h host'" << std::endl;
		return -1;
	}

	if(port == 0){
		std::cout << "port not specified. use '-p port'" << std::endl;
		return -1;
	}

	std::cout << "mom is running on [" << host << ":" << port << "]" << std::endl;

	// startup
	auto server = std::make_shared<TcpServer>(host, port, std::make_shared<ServerHandler>());
	server->startup();

	RUN_UV_DEFAULT_LOOP();
	server->shutdown();

	return 0;
}


