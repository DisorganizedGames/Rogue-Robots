#pragma once
#include "RenderResourceHandles.h"
#include <windows.h>

namespace DOG::gfx
{
	class RenderDevice;
	class Swapchain;

	class D2DBackend
	{
		public:
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Render() = 0;

		virtual ~D2DBackend() {};
	};
}