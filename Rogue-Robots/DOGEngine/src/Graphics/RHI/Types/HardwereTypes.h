#pragma once

namespace DOG::gfx
{
	struct Monitor
	{
		DXGI_OUTPUT_DESC1 output;
		std::vector<DXGI_MODE_DESC> modes;
	};
}