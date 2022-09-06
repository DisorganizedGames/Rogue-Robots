#pragma once

class HR
{
	HRESULT hr;
public:
	/* implicit constructor */
	constexpr HR(HRESULT hr = 0) noexcept : hr(hr) {}

	void try_fail(const std::string_view message)
	{
		if (this->failed())
		{
			std::cerr << message << "\n" << "HRESULT " << std::hex << hr << ": ";
			this->print();
			assert(false);
		}
	};

	constexpr bool succeeded()
	{
		return SUCCEEDED(hr);
	}

	constexpr bool failed()
	{
		return FAILED(hr);
	}

	void print()
	{
		char out[64];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			out, 64, nullptr);

		std::cout << out;
	}
};