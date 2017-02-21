#include<functional>
#include "tcp_server.h"
#include "tcp_client.h"

char data[] = "Hello, world!";
const char * default_ip = "192.168.1.17";
TcpClient * client;

void write();

void write_cb(int status) {
	if (!status) {
		write();
	}
}

void write(){
	client->write(data, strlen(data), write_cb);
}

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		printf("%s %s\n", argv[0], argv[1]);

		if (strcmp(argv[1], "s") == 0) {
			TcpServer server;
			server.start("0.0.0.0", 5001);
		}
		else if (strcmp(argv[1], "c") == 0) {
			client = new TcpClient(argc > 2 ? argv[2] : default_ip, 5001);
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
