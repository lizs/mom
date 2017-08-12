#include "property_test.h"
#include "signal_test.h"
#include "any_test.h"
#include "net_test.h"

int main(int argc, char** argv) {

	//srand(time(0));

	//std::vector<cbuf_ptr_t> pcbs;
	//for(auto i = 0; i < 1000; ++i) {
	//	auto len = rand() % 8 * 1023 + 1;
	//	auto pcb = alloc_cbuf(len);
	//	pcb->move_tail(len);
	//	pcbs.push_back(pcb);
	//}

	//pcbs.clear();
	//return 0;
	
	if(argc > 1) {
		VK::Logger::instance().start(argv[1]);

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
			if (argc > 2) {
				auto cnt = atoi(argv[2]);
				run_client_test(cnt);
			}
			else
				run_client_test(1);
		}
		else if (strcmp(argv[1], "server") == 0) {
			run_server_test();
		}
	}

	return 0;
}
