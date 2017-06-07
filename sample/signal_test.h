#pragma once

#include <iostream>
#include <functional>
#include <observable/slot.h>
#include <observable/signal.h>
#include <observable/signal2.h>

using namespace VK;

static void print() {
	std::cout << "Hello, free function!" << std::endl;
}

struct Test {
	void print() {
		std::cout << "Hello, member function!" << std::endl;
	}
};

static void run_signal2_test() {
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

static void run_signal_test() {
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