#pragma once

int main(int, char**)
{
	HR hr = CoInitialize(nullptr);
	hr.try_fail("Failed to initialize COM");

	std::cout << "Hello World!" << std::endl;

	CoUninitialize();
	return 0;
}