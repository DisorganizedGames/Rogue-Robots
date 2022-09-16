#pragma once
#include "../SwapChain.h"
#include "CommonIncludes_DX12.h"

#include <windows.h>

namespace DOG::gfx
{
	class RenderDevice_DX12;

	class Swapchain_DX12 : public Swapchain
	{
	public:
		Swapchain_DX12(RenderDevice_DX12* device, HWND hwnd, u8 num_buffers, bool debug_on);
		~Swapchain_DX12();

		// Public interface
		Texture GetNextDrawSurface();
		u8 GetNextDrawSurfaceIdx();
		void SetClearColor(const std::array<float, 4>& clear_color);

		Texture GetBuffer(u8 idx);
		DXGI_FORMAT GetBufferFormat() const;
		std::vector<DXGI_MODE_DESC> GetModeDescs(DXGI_FORMAT format) const;
		DXGI_OUTPUT_DESC1 GetOutputDesc() const;

		void Present(bool vsync);

		// Implementation interface
	public:
		HWND GetHWND() const { return m_hwnd; }


	private:
		bool IsTearingSupported();

	private:
		RenderDevice_DX12* m_device{ nullptr };
		HWND m_hwnd;
		ComPtr<IDXGISwapChain3> m_sc;
		DXGI_FORMAT m_scFormat;

		std::vector<Texture> m_buffers;

		bool m_tearingIsSupported{ false };
	};
}