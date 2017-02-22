#include<functional>
#include "tcp_server.h"
#include "tcp_client.h"
#include <session.h>
#include "packers.h"

using namespace Bull;
char data[1024] = "Hello, world!";
const char * default_ip = "192.168.1.17";
using Client = TcpClient<Session<SerialPacker<64>>>;
using Server = TcpServer<Session<SerialPacker<64>>>;
Client * client;
Server * server;

void write();

void write_cb(int status) {
	if (!status) {
		write();
	}
}

void write(){
	client->write(data, strlen(data) + 1, write_cb);
}

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		printf("%s %s\n", argv[0], argv[1]);

		if (strcmp(argv[1], "s") == 0) {
			server = new Server();
			server->start("0.0.0.0", 5001);

			delete server;
		}
		else if (strcmp(argv[1], "c") == 0) {
			client = new Client(argc > 2 ? argv[2] : default_ip, 5001);
			client->connect([](int status) {
				if (!status)
					write();
			});

			client->close();
			delete client;
		}
		return 0;
	}

	return -1;
}
