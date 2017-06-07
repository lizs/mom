#include "property_test.h"
#include "signal_test.h"
#include "any_test.h"
#include "net_test.h"

int main(int argc, char** argv) {
	if(argc > 1) {
		if(strcmp(argv[1], "any") == 0) {
			run_any_test();
		}
		else if (strcmp(argv[1], "signal") == 0) {
			run_signal_test();
		}
		else if (strcmp(argv[1], "signal2") == 0) {
			run_signal2_test();
		}
		else if (strcmp(argv[1], "value") == 0) {
			run_value_test();
		}
		else if (strcmp(argv[1], "property") == 0) {
			run_property_test();
		}
		else if (strcmp(argv[1], "client") == 0) {
			run_client_test();
		}
		else if (strcmp(argv[1], "server") == 0) {
			run_server_test();
		}
	}

	return 0;
}
