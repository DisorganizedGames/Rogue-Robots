#pragma once
#include "src/Application.h"
int main(int, char**)
{
	HR hr = CoInitialize(nullptr);
	hr.try_fail("Failed to initialize COM");

	std::cout << "Hello World!" << std::endl;

	CoUninitialize();

	DOG::Application app;


	return 0;
}