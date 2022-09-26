#pragma once
#include "RenderResourceHandles.h"
#include <windows.h>

namespace DOG::gfx
{
	class RenderDevice;
	class Swapchain;

	class d2dBackend
	{
		public:
		virtual void BeginFrame(RenderDevice* rd, Swapchain* sc) = 0;
		virtual void EndFrame(RenderDevice* rd, Swapchain* sc) = 0;
		virtual void Render(RenderDevice* rd, Swapchain* sc) = 0;

		virtual ~d2dBackend() {};
	};
}