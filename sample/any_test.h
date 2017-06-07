#pragma once
#include <iostream>
#include <data/any.h>
#include <string>

using namespace VK;

static void run_any_test() {
	std::cout << "run_any_test" << std::endl;

	VK::any  a;
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
