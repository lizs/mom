#include <iostream>
#include <observable/signal.h>
#include <observable/signal2.h>
#include <data/value.h>
#include <data/property.h>
#include <watch.h>

using namespace VK;

enum Pid {
	One,
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
	Value<int> intger(One, 1);
	intger.conn([](val_id_t vid, const int& val) {
		std::cout << "conn : Value " << vid << " Changed to : " << val << std::endl;
	});
	intger = 2;
	intger = 3;
	intger = 4;
	// no longer callback here
	intger = 4;
}

void run_property_test() {
	// 初始属性包，初值为0
	Property<int> intgers({ Pid::One, Pid::Two}, 0);
	auto cid2 = intgers.conn(One, [](val_id_t vid, const int& val) {
		                         std::cout << "conn : Property " << vid << " Changed to : " << val << std::endl;
	                         });

	auto gcid = intgers.conn_all([](val_id_t vid, const int& val) {
		std::cout << "conn_all : Property " << vid << " Changed to : " << val << std::endl;
	});

	intgers.set(One, 1);
	intgers.disconn(One, cid2);
	intgers.set(One, 10);
	intgers.set(Two, 20);
	intgers.disconn_all(gcid);

	Watch watch;
	watch.start();
	// 测试100W订阅者
	for (auto i = 0; i < 1000000; ++i) {
		intgers.conn_all([](val_id_t vid, const int& val) {
			//std::cout << "conn_all : Property " << vid << " Changed to : " << val << std::endl;
		});
	}
	watch.stop();	// 229ms
	std::cout << "Elapsed " << watch.elapsed_milli_seconds() << " ms" << std::endl;


	watch.start();
	intgers.set(Two, 20);
	watch.stop();	// 25ms
	std::cout << "Elapsed " << watch.elapsed_milli_seconds() << " ms" << std::endl;
}

int main(int argc, char** argv) {
	run_signal2_test();
	run_signal_test();
	run_value_test();
	run_property_test();

	return 0;
}
