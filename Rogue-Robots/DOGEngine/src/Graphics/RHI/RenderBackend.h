#pragma once

namespace DOG::gfx
{
	class RenderDevice;
	
	class RenderBackend
	{
	public:
		virtual RenderDevice* CreateDevice(UINT numBackBuffers) = 0;
		virtual ~RenderBackend() = default;
	};
}
