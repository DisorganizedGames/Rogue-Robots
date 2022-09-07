#pragma once

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

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