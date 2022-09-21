#pragma once
#include <d3d12.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <dwrite.h>
#include "CommonIncludes_DX12.h"
#include "../d2dBackend.h"

namespace DOG::gfx
{
	class RenderDevice;
	class Swapchain;

	class d2dBackend_DX12 final : public d2dBackend
	{
	public:
		d2dBackend_DX12(RenderDevice* rd, Swapchain* sc, u32 maxFramesInFlight, HWND hwnd);
		~d2dBackend_DX12();

		// Public interface
		void BeginFrame();
		void EndFrame();
		void Render(RenderDevice* rd,Swapchain* sc, CommandList cmdl);
		bool WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		// Implementation interface
	public:
		//void Render(ID3D12GraphicsCommandList4* cmdl);

		/*
		
			// At m_rd->BeginFrame();
			ImGUI->BeginFrame();

			m_rd->Cmd_RenderImGUI(ImGUI)

			// At Renderer->EndFrame();
			ImGUI->EndFrame();
		
		*/

	private:
      ComPtr<ID3D11Device> m_d;
		ComPtr<ID3D11DeviceContext> m_dc;
		ComPtr<IDXGIDevice> m_dxd;
		ComPtr<ID3D11On12Device> m_11on12d;
		ComPtr<ID2D1Factory3> m_factory;
		ComPtr<ID2D1Device> m_2dd;
		ComPtr<ID2D1DeviceContext> m_2ddc;
		ComPtr<IDWriteFactory> m_dwritwf;
		ComPtr<ID3D12Resource> m_renderTargets[2];
		ComPtr<ID3D11Resource> m_wrappedBackBuffers[2];
		ComPtr<IDXGISurface> surface;
		ComPtr<ID2D1Bitmap1> m_d2dRenderTargets[2];
		ComPtr<ID2D1SolidColorBrush> brush;
      ComPtr<IDWriteTextFormat> format;
		ComPtr<ID3D12CommandAllocator> m_commandAllocators[2];

	};
}