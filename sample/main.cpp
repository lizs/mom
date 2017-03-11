#include <iostream>
#include <observable/signal.h>
#include <observable/signal2.h>
#include <data/value.h>
#include <data/property.h>

void print() {
	std::cout << "Hello, free function!" << std::endl;
}

struct Test {
	void print() {
		std::cout << "Hello, member function!" << std::endl;
	}
};

int main(int argc, char** argv) {
	using namespace VK;
	
	Signal2<void(*)()> sig1;
	auto slot1 = make_slot(print);
	sig1.connect(slot1);
	sig1.emit();
	sig1.disconnect(make_slot(print));
	sig1.emit();

	Test test;
	Signal2<void(Test::*)()> sig2;
	auto slot2 = make_slot(&Test::print, test);
	sig2.connect(slot2);
	sig2.emit();
	sig2.disconnect(slot2);
	sig2.emit();

	Signal<void()> sig3;
	auto cid = sig3.conn(print);
	sig3.conn(std::bind(&Test::print, test));
	sig3.conn([]() {
		std::cout << "Hello, lambda!" << std::endl;
	});
	sig3();
	sig3.disconn(cid);
	sig3();
	sig3.disconn_all();
	sig3();

	enum Pid {
		One,
		Two,
		Three,
	};

	Value<int> intger(One, 1);
	intger.conn([](val_id_t vid, const int& val) {
		std::cout << "conn : Value " << vid << " Changed to : " << val << std::endl;
	});
	intger = 2;
	intger = 3;
	intger = 4;
	// no longer callback here
	intger = 4;

	Property<int> intgers({One, Pid::Two, Pid::Three}, 0);
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
	intgers.set(Three, 30);

	auto& one = intgers.get(One);
	auto& two = intgers.get(Two);
	auto& three = intgers.get(Three);

	return 0;
}
