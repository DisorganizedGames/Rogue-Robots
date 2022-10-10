#pragma once
#include "RHITypes.h"

/*
	Interface used for pImpl, fully decouple impl. (header)
*/
namespace DOG::gfx
{
	class Swapchain
	{
	public:
		virtual Texture GetNextDrawSurface() = 0;
		virtual u8 GetNextDrawSurfaceIdx() = 0;
		virtual void Present(bool vsync) = 0;
		virtual void SetClearColor(const std::array<float, 4>& clear_color) = 0;
		virtual void OnResize(u32 clientWidth, u32 clientHeight) = 0;
		virtual bool SetFullscreenState(bool fullscreen, DXGI_MODE_DESC mode) = 0;
		virtual bool GetFullscreenState() const = 0;

		virtual Texture GetBuffer(u8 idx) = 0;
		virtual DXGI_FORMAT GetBufferFormat() const = 0;

		virtual ~Swapchain() {}
	};
}
