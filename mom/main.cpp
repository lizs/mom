#include<functional>
#include "tcp_server.h"
#include "tcp_client.h"

char data[] = "Hello, world!";
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
		if (strcmp(argv[1], "s") == 0) {
			TcpServer server;
			server.start("0.0.0.0", 5001);
		}
		else if (strcmp(argv[1], "c") == 0) {
			client = new TcpClient();
			client->start("127.0.0.1", 5001, [](int status) {
				if(!status)
					write();
			});

			int n;
			scanf("%d", &n);

			client->shutdown();
			delete client;
		}
		return 0;
	}

	return -1;
}
