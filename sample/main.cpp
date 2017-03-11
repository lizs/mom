#include <iostream>
#include <observable/signal.h>
#include <observable/signal2.h>
#include <data/value.h>

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
	auto cid = sig3.connect(print);
	sig3.connect(std::bind(&Test::print, test));
	sig3.connect([]() {
		std::cout << "Hello, lambda!" << std::endl;
	});
	sig3();
	sig3.disconnect(cid);
	sig3();
	sig3.disconnect_all();
	sig3();

	Value<int> intger = 1;
	intger.connect([](const int& val) {
		std::cout << "Changed to : " << val << std::endl;
	});
	intger = 2;
	intger = 3;
	intger = 4;
	intger = 4;

	return 0;
}
