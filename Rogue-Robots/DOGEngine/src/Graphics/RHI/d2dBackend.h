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
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Render(RenderDevice* rd, Swapchain* sc, CommandList cmdl) = 0;
		//virtual bool WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

		virtual ~d2dBackend() {};
	};
}