#include <iostream>
#include <observable/signal.h>
#include <observable/signal2.h>
#include <data/value.h>
#include <data/property.h>
#include <data/any.h>
#include <watch.h>
#include <string>

using namespace VK;

enum Pid {
	One = 1,
	Two,
	Three,
};

void print() {
	std::cout << "Hello, free function!" << std::endl;
}

struct Test {
	void print() {
		std::cout << "Hello, member function!" << std::endl;
	}
};

void run_signal2_test() {
	std::cout << "run_signal2_test" << std::endl;

	Signal2<void(*)()> sig1;
	auto slot1 = make_slot(print);
	sig1.connect(slot1);
	sig1.emit();
	sig1.disconnect(make_slot(print));
	sig1.emit();

	Signal2<void(Test::*)()> sig2;
	Test test;
	auto slot2 = make_slot(&Test::print, test);
	sig2.connect(slot2);
	sig2.emit();
	sig2.disconnect(slot2);
	sig2.emit();
}

void run_signal_test() {
	std::cout << "run_signal_test" << std::endl;

	Signal<void()> sig;
	auto cid = sig.conn(print);

	Test test;
	sig.conn(std::bind(&Test::print, test));
	sig.conn([]() {
		std::cout << "Hello, lambda!" << std::endl;
	});
	sig();
	sig.disconn(cid);
	sig();
	sig.disconn_all();
	sig();
}

void run_value_test() {
	std::cout << "run_value_test" << std::endl;

	Value intger(One, 1);
	Value intger1(Two, 2);
	
	intger.conn([](val_id_t vid, const Value& val) {
		std::cout << "conn : Value " << vid << " Changed to : " << val.to<int>() << std::endl;
	});

	// will throw exception
	// can only be assigned with same type
	//intger = intger1;

	intger = 2;
	intger = 3;
	intger = 4;
	// no longer callback here
	intger = 4;
}

void run_any_test() {
	std::cout << "run_any_test" << std::endl;

	any  a;
	a = 1;
	std::cout << any_cast<int>(a) << std::endl;
	a = 1.0f;
	std::cout << any_cast<float>(a) << std::endl;
	a = std::string("Hello world!");
	std::cout << any_cast<std::string>(a) << std::endl;
	a = "Hello world!";
	std::cout << any_cast<const char *>(a) << std::endl;

	struct Test {
		int a;
		float b;
		std::string c;
	};
	Test test = { 1, 1.0f, "Hello world!" };
	a = test;
	auto out = any_cast<Test>(a);
	printf("%d %f %s\n", out.a, out.b, out.c.c_str());
}

void run_property_test() {
	std::cout << "run_property_test" << std::endl;

	// 初始属性包
	Property pack;
	pack.add<int>(One, 0);
	pack.add<float>(Two, 0);
	pack.add<std::string>(Three, "Hello world!");
	
	auto cid2 = pack.conn(One, [](val_id_t vid, const Value& val) {
		                      std::cout << "conn : Property " << vid << " Changed to : ";
		                      switch (static_cast<Pid>(vid)) {
			                      case One:
				                      std::cout << val.to<int>() << std::endl;
				                      break;
			                      default:
				                      break;
		                      }
	                      });

	auto gcid = pack.conn_all([](val_id_t vid, const Value& val) {
		std::cout << "conn_all : Property " << vid << " Changed to : ";
		switch (static_cast<Pid>(vid)) {
			case One:
				std::cout << val.to<int>() << std::endl;
				break;
			case Two:
				std::cout << val.to<float>() << std::endl;
				break;
			case Three:
				std::cout << val.to<std::string>() << std::endl;
				break;
			default:
				break;
		}
	});

	pack.set(One, 1);
	pack.disconn(One, cid2);
	pack.set(One, 10);
	pack.set(Two, 20.0f);
	pack.set<std::string>(Three, "Hello Property!");
	pack.disconn_all(gcid);

	//Watch watch;
	//watch.start();
	//// 测试100W订阅者
	//for (auto i = 0; i < 1000000; ++i) {
	//	pack.conn_all([](val_id_t vid, const Value& val) {
	//		//std::cout << "conn_all : Property " << vid << " Changed to : " << val << std::endl;
	//	});
	//}
	//watch.stop();	// 229ms
	//std::cout << "Elapsed " << watch.elapsed_milli_seconds() << " ms" << std::endl;


	//watch.start();
	//pack.set(Two, 20.0f);
	//watch.stop();	// 0ms
	//std::cout << "Elapsed " << watch.elapsed_milli_seconds() << " ms" << std::endl;
}

int main(int argc, char** argv) {
	//run_any_test();
	//run_signal2_test();
	//run_signal_test();
	//run_value_test();
	run_property_test();

	return 0;
}
