#include "Swapchain_DX12.h"

#include "RenderDevice_DX12.h"

namespace DOG::gfx
{
	Swapchain_DX12::Swapchain_DX12(RenderDevice_DX12* device, HWND hwnd, u8 num_buffers, bool debug_on) :
		m_hwnd(hwnd),
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
		scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		scd.Flags |= m_tearingIsSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

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

		std::tie(m_windowedClientWidth, m_windowedClientHeight) = GetSwapchainWidthAndHeight();
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

	void Swapchain_DX12::OnResize(u32 clientWidth, u32 clientHeight)
	{
		m_device->Flush();

		HRESULT hr{ S_OK };
		DXGI_SWAP_CHAIN_DESC1 swapDesc;
		hr = m_sc->GetDesc1(&swapDesc);
		HR_VFY(hr);

		for (const auto& tex : m_buffers)
			m_device->FreeTexture(tex);


		hr = m_sc->ResizeBuffers(0, clientWidth, clientHeight, swapDesc.Format, swapDesc.Flags);


		for (u32 i = 0; i < m_buffers.size(); ++i)
		{
			ComPtr<ID3D12Resource> buffer;
			hr = m_sc->GetBuffer(i, IID_PPV_ARGS(buffer.GetAddressOf()));
			HR_VFY(hr);

			m_buffers[i] = (m_device->RegisterSwapchainTexture(buffer));
		}

		HR_VFY(hr);
	}

	bool Swapchain_DX12::GetFullscreenState() const
	{
		BOOL state;
		HRESULT hr{ S_OK };
		hr = m_sc->GetFullscreenState(&state, nullptr);
		HR_VFY(hr);
		return static_cast<bool>(state);
	}

	bool Swapchain_DX12::SetFullscreenState(bool fullscreen, DXGI_MODE_DESC mode)
	{
		if (fullscreen)
		{
			EnterFullscreen(mode);
		}
		else
		{
			ExitFullscreen();
		}

		m_isFullscreen = GetFullscreenState();
		assert(m_isFullscreen == fullscreen);
		return true;
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

	std::vector<DXGI_MODE_DESC> Swapchain_DX12::GetModeDescs(DXGI_FORMAT format) const
	{
		IDXGIOutput* output;
		HRESULT hr = m_sc->GetContainingOutput(&output);
		HR_VFY(hr);

		UINT numModes = 0;
		hr = output->GetDisplayModeList(format, 0, &numModes, nullptr);
		HR_VFY(hr);

		std::vector<DXGI_MODE_DESC> modes;
		modes.resize(numModes);

		hr = output->GetDisplayModeList(format, 0, &numModes, modes.data());
		HR_VFY(hr);
		output->Release();
		return modes;
	}

	DXGI_MODE_DESC Swapchain_DX12::GetDefaultDisplayModeDesc() const
	{
		auto rect = GetOutputDesc().DesktopCoordinates;
		DXGI_MODE_DESC modeDesc{};

		DXGI_MODE_DESC prefModeDesc{};
		prefModeDesc.Width = abs(rect.left - rect.right);
		prefModeDesc.Height = abs(rect.bottom - rect.top);
		prefModeDesc.Format = GetBufferFormat();

		IDXGIOutput* output = nullptr;
		HRESULT hr = m_sc->GetContainingOutput(&output);
		HR_VFY(hr);

		hr = output->FindClosestMatchingMode(&prefModeDesc, &modeDesc, nullptr);
		HR_VFY(hr);
		output->Release();

		return modeDesc;
	}

	DXGI_OUTPUT_DESC1 Swapchain_DX12::GetOutputDesc() const
	{
		IDXGIOutput* output;
		HRESULT hr = m_sc->GetContainingOutput(&output);
		HR_VFY(hr);

		IDXGIOutput6* output6;
		hr = output->QueryInterface(__uuidof(IDXGIOutput6), reinterpret_cast<void**>(&output6));
		HR_VFY(hr);

		DXGI_OUTPUT_DESC1 desc{};
		hr = output6->GetDesc1(&desc);
		HR_VFY(hr);

		output->Release();
		output6->Release();
		return desc;
	}

	std::pair<u32, u32> Swapchain_DX12::GetSwapchainWidthAndHeight() const
	{
		HRESULT hr{ S_OK };
		DXGI_SWAP_CHAIN_DESC1 swapDesc;
		hr = m_sc->GetDesc1(&swapDesc);
		HR_VFY(hr);
		return std::make_pair<u32, u32>(static_cast<u32>(swapDesc.Width), static_cast<u32>(swapDesc.Height));
	}

	void Swapchain_DX12::Present(bool vsync)
	{
		u32 flags{ 0 };
		if (!vsync && m_tearingIsSupported && !m_isFullscreen)
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

	bool Swapchain_DX12::EnterFullscreen(DXGI_MODE_DESC mode)
	{
		if (GetFullscreenState())
		{
			// We are already in fullscreen, set the new mode and return

			HRESULT hr{ S_OK };
			hr = m_sc->ResizeTarget(&mode);
			if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
			{
				return false;
			}
			HR_VFY(hr);
			return true;
		}
		// Save width and height so that we might reset the window to them later on
		std::tie(m_windowedClientWidth, m_windowedClientHeight) = GetSwapchainWidthAndHeight();

		HRESULT hr{ S_OK };
		hr = m_sc->ResizeTarget(&mode);
		if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			return false;
		}
		HR_VFY(hr);

		hr = m_sc->SetFullscreenState(TRUE, nullptr);
		if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			return false;
		}
		else if (hr == DXGI_STATUS_MODE_CHANGE_IN_PROGRESS)
		{
			return false;
		}
		HR_VFY(hr);

		hr = m_sc->ResizeTarget(&mode);
		if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			return false;
		}
		HR_VFY(hr);

		OnResize(mode.Width, mode.Height);

		return true;
	}
	bool Swapchain_DX12::ExitFullscreen()
	{
		if (!GetFullscreenState()) return true;

		HRESULT hr{ S_OK };
		hr = m_sc->SetFullscreenState(FALSE, nullptr);
		if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			return false;
		}
		else if (hr == DXGI_STATUS_MODE_CHANGE_IN_PROGRESS)
		{
			return false;
		}
		HR_VFY(hr);

		if (m_windowedClientWidth != 0 && m_windowedClientHeight != 0)
		{
			DXGI_MODE_DESC mode{};
			mode.Width = m_windowedClientWidth;
			mode.Height = m_windowedClientHeight;
			hr = m_sc->ResizeTarget(&mode);
			if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
			{
				return false;
			}
			HR_VFY(hr);
		}
		return true;
	}
}