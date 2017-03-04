#include <iostream>
#include <boost/signals2.hpp>
#include <observable/signal.h>

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
	
	Signal<void(*)()> sig1;
	auto slot1 = make_slot(print);
	sig1.connect(slot1);
	sig1.emit();
	sig1.disconnect(make_slot(print));
	sig1.emit();

	Test test;
	Signal<void(Test::*)()> sig2;
	auto slot2 = make_slot(&Test::print, test);
	sig2.connect(slot2);
	sig2.emit();
	sig2.disconnect(slot2);
	sig2.emit();

	return 0;
}
