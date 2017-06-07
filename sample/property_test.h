#pragma once
#include <iostream>
#include <data/value.h>
#include <data/property.h>
#include <string>

using namespace VK;

enum Pid {
	One = 1,
	Two,
	Three,
};

static void run_value_test() {
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



static void run_property_test() {
	std::cout << "run_property_test" << std::endl;

	// ³õÊ¼ÊôÐÔ°ü
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
}