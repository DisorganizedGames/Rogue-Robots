#pragma once
#include <d3d12.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <dwrite.h>
#include <vector>
#include <memory>
#include "CommonIncludes_DX12.h"
#include "Utilities/DX12DescriptorManager.h"
#include "../d2dBackend.h"

namespace DOG::gfx
{
	class RenderDevice;
	class Swapchain;

	class D2DBackend_DX12 final : public D2DBackend
	{
	public:
		D2DBackend_DX12(RenderDevice* rd, Swapchain* sc, u_int numBuffers, HWND hwnd);
		~D2DBackend_DX12();
		RenderDevice* rd;
		Swapchain* sc;

		// Public interface
		void BeginFrame() override;
		void EndFrame() override;
		void Render() override;

		ComPtr<ID3D11Device> m_d;
		ComPtr<ID3D11DeviceContext> m_dc;
		ComPtr<IDXGIDevice> m_dxd;
		ComPtr<ID3D11On12Device> m_11on12d;
		ComPtr<ID2D1Factory3> m_factory;
		ComPtr<ID2D1Device> m_2dd;
		ComPtr<ID2D1DeviceContext> m_2ddc;
		ComPtr<IDWriteFactory> m_dwritwf;
		std::vector<ComPtr<ID3D12Resource>> m_renderTargets;
		std::vector<ComPtr<ID3D11Resource>> m_wrappedBackBuffers;
		ComPtr<IDXGISurface> surface;
		std::vector<ComPtr<ID2D1Bitmap1>> m_d2dRenderTargets;
		ComPtr<ID2D1SolidColorBrush> brush;
      ComPtr<IDWriteTextFormat> format, bformat;
		ComPtr<ID3D12CommandAllocator> m_commandAllocators[2];
		std::unique_ptr<DX12DescriptorManager> m_descriptorMgr;

	private:
      u_int m_numBuffers;
		DX12DescriptorChunk rtvHandle;

	};
}