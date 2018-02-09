#include "property_test.h"
#include "signal_test.h"
#include "any_test.h"
#include "net_test.h"

static std::map<std::string, std::string> parse_command_line(int argc, char* argv[]) {
	std::map<std::string, std::string> dic;

	std::string key = "";
	for (auto i = 0; i < argc; ++i) {
		auto str = std::string(argv[i]);
		if (str.find("-") != std::string::npos) {
			key.clear();
			key.reserve(str.length());
			for (auto c : str) {
				if (c != '-') {
					key += (c);
				}
			}
			dic[key] = "";
		}
		else {
			dic[key] = argv[i];
		}
	}

	return std::move(dic);
}

int main(int argc, char** argv) {	
	// ½âÎöÃüÁîĞĞ
	auto dic = std::move(parse_command_line(argc, argv));
	for (auto& kv : dic) {
		if (kv.first == "c" || kv.first == "config") {
		}
		else if (kv.first == "ll" || kv.first == "loglevel") {
		}
		else if (kv.first == "i" || kv.first == "id") {
		}
		else if (kv.first == "ds" || kv.first == "distributestep") {
		}
		else if (kv.first == "af" || kv.first == "address4frontend") {
		}
		else if (kv.first == "ab" || kv.first == "address4backend") {
		}
	}


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
			run_server();
		}
	}

	return 0;
}
