#include "Swapchain_DX12.h"

#include "RenderDevice_DX12.h"

namespace DOG::gfx
{
	Swapchain_DX12::Swapchain_DX12(RenderDevice_DX12* device, HWND hwnd, u8 num_buffers, bool debug_on) :
		m_device(device),
		m_scFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		assert(num_buffers >= 2);

		HRESULT hr{ S_OK };

		// Swapchain may force a flush on the associated queue (requires direct queue, to be specific)
		// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/D3D12HelloTriangle.cpp#L95
		// https://docs.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforhwnd

		DXGI_SWAP_CHAIN_DESC1 scd{};
		scd.Width = 0;
		scd.Height = 0;
		scd.BufferCount = num_buffers;
		scd.Format = m_scFormat;				// Requires manual gamma correction before presenting
		scd.SampleDesc.Count = 1;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scd.Flags = m_tearingIsSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		// Grab IDXGIFactory4 for CreateSwapChainForHwnd
		ComPtr<IDXGIFactory2> fac;
		UINT factoryFlags = debug_on ? DXGI_CREATE_FACTORY_DEBUG : 0;
		hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(fac.GetAddressOf()));
		HR_VFY(hr);
		ComPtr<IDXGIFactory4> fac4;
		hr = fac.As(&fac4);
		HR_VFY(hr);

		ComPtr<IDXGISwapChain1> sc;
		hr = fac->CreateSwapChainForHwnd(
			m_device->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
			hwnd,
			&scd,
			nullptr,			// no fullscreen
			nullptr,			// no output restrictions
			sc.GetAddressOf());
		HR_VFY(hr);

		// Grab SwapChain4 to get access to GetBackBufferIndex
		hr = sc.As(&m_sc);
		HR_VFY(hr);

		// Disable alt-enter for now
		hr = fac->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
		HR_VFY(hr);

		for (uint32_t i = 0; i < num_buffers; ++i)
		{
			ComPtr<ID3D12Resource> buffer;
			hr = m_sc->GetBuffer(i, IID_PPV_ARGS(buffer.GetAddressOf()));
			HR_VFY(hr);

			m_buffers.push_back(m_device->RegisterSwapchainTexture(buffer));
		}
	}

	Swapchain_DX12::~Swapchain_DX12()
	{
		for (auto bb : m_buffers)
			m_device->FreeTexture(bb);
	}
	Texture Swapchain_DX12::GetNextDrawSurface()
	{
		u32 index = m_sc->GetCurrentBackBufferIndex();
		return m_buffers[index];
	}
	u8 Swapchain_DX12::GetNextDrawSurfaceIdx()
	{
		return (u8)m_sc->GetCurrentBackBufferIndex();
	}
	void Swapchain_DX12::SetClearColor(const std::array<float, 4>& clear_color)
	{
		for (const auto& bb : m_buffers)
			m_device->SetClearColor(bb, clear_color);
	}

	Texture Swapchain_DX12::GetBuffer(u8 idx)
	{
		assert(idx < m_buffers.size());
		return m_buffers[idx];
	}

	DXGI_FORMAT Swapchain_DX12::GetBufferFormat() const
	{
		return m_scFormat;
	}

	void Swapchain_DX12::Present(bool vsync)
	{
		u32 flags{ 0 };
		if (!vsync && m_tearingIsSupported)
			flags |= DXGI_PRESENT_ALLOW_TEARING;

		HRESULT hr{ S_OK };
		hr = m_sc->Present(vsync, flags);
		HR_VFY(hr);
	}
	bool Swapchain_DX12::IsTearingSupported()
	{
		BOOL allowed = FALSE;
		ComPtr<IDXGIFactory4> fac4;
		if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&fac4))))
		{
			ComPtr<IDXGIFactory5> fac5;
			if (SUCCEEDED(fac4.As(&fac5)))
			{
				auto hr = fac5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowed, sizeof(allowed));
				allowed = SUCCEEDED(hr) && allowed;
			}
		}
		return allowed;
	}
}