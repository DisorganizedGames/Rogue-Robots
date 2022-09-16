#pragma once

#include <windows.h>

namespace DOG::gfx
{
	class ImGUIBackend
	{
	public:
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual bool WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

		virtual ~ImGUIBackend() {};
	};
}