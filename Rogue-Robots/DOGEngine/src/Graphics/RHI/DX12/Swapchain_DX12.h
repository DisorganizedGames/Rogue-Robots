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

		void OnResize(u32 clientWidth, u32 clientHeight) override;
		bool GetFullscreenState() const override;
		bool SetFullscreenState(bool fullscreen, DXGI_MODE_DESC mode) override;

		Texture GetBuffer(u8 idx);
		DXGI_FORMAT GetBufferFormat() const;
		std::vector<DXGI_MODE_DESC> GetModeDescs(DXGI_FORMAT format) const;
		DXGI_MODE_DESC GetDefaultDisplayModeDesc() const;
		DXGI_OUTPUT_DESC1 GetOutputDesc() const;
		std::pair<u32, u32> GetSwapchainWidthAndHeight() const;
		void Present(bool vsync);

		// Implementation interface
	public:
		HWND GetHWND() const { return m_hwnd; }


	private:
		bool IsTearingSupported();

		bool EnterFullscreen(DXGI_MODE_DESC mode);
		bool ExitFullscreen();

	private:
		RenderDevice_DX12* m_device{ nullptr };
		HWND m_hwnd;
		ComPtr<IDXGISwapChain3> m_sc;
		DXGI_FORMAT m_scFormat;

		std::vector<Texture> m_buffers;

		bool m_tearingIsSupported{ false };
		u32 m_windowedClientWidth{ 0 };
		u32 m_windowedClientHeight{ 0 };
	};
}