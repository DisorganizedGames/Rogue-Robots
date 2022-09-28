#include "d2dBackend_DX12.h"


#include "RenderDevice_DX12.h"
#include "Swapchain_DX12.h"


DOG::gfx::d2dBackend_DX12::d2dBackend_DX12(RenderDevice* rd, Swapchain* sc, u_int numBuffers, HWND hwnd)
{
    // This is guaranteed since we have no other backends than DX12
    auto rd12 = (RenderDevice_DX12*)rd;
    auto sc12 = (Swapchain_DX12*)sc;

    // Create an 11 device wrapped around the 12 device and share
    // 12's command queue.
    auto test = rd12->_GetQueue();
    HRESULT hr = D3D11On12CreateDevice(
        rd12->GetDevice(),
        D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        NULL,
        1u,
        reinterpret_cast<IUnknown**>(&test),
        1,
        0,
        &m_d,
        &m_dc,
        nullptr
    );
    HR_VFY(hr);
    hr = m_d->QueryInterface(IID_PPV_ARGS(&m_11on12d));

    // Query the 11On12 device from the 11 device.
    HR_VFY(hr);

    {
        D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
        D2D1_FACTORY_OPTIONS f_opt = {
            .debugLevel = D2D1_DEBUG_LEVEL_INFORMATION
        };
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &f_opt, (void**)&m_factory);
        HR_VFY(hr);
        hr = m_11on12d.As(&m_dxd);
        HR_VFY(hr);
        hr = m_factory->CreateDevice(m_dxd.Get(), m_2dd.GetAddressOf());
        HR_VFY(hr);
        hr = m_2dd->CreateDeviceContext(deviceOptions, &m_2ddc);
        HR_VFY(hr);
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&m_dwritwf);
        HR_VFY(hr);
    }


    float dpi = GetDpiForWindow(hwnd);
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        dpi,
        dpi);

    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rd12->GetReservedRTV();

        // Create a RTV, D2D render target, and a command allocator for each frame.
        for (UINT n = 0; n < numBuffers; n++)
        {
            m_renderTargets[n] = sc12->_GetBuffer(n);
            rd12->GetDevice()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);

            // Create a wrapped 11On12 resource of this back buffer. Since we are 
            // rendering all D3D12 content first and then all D2D content, we specify 
            // the In resource state as RENDER_TARGET - because D3D12 will have last 
            // used it in this state - and the Out resource state as PRESENT. When 
            // ReleaseWrappedResources() is called on the 11On12 device, the resource 
            // will be transitioned to the PRESENT state.
            D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
            hr = m_11on12d->CreateWrappedResource(
                m_renderTargets[n].Get(),
                &d3d11Flags,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT,
                IID_PPV_ARGS(&m_wrappedBackBuffers[n])
            );
            HR_VFY(hr);

            // Create a render target for D2D to draw directly to this back buffer.

            hr = m_wrappedBackBuffers[n].As(&surface);
            HR_VFY(hr);
            hr = m_2ddc->CreateBitmapFromDxgiSurface(
                surface.Get(),
                bitmapProperties,
                m_d2dRenderTargets[n].GetAddressOf()
            );
            HR_VFY(hr);
        }
        {
            ComPtr<IDWriteFactory> dwritef;
            hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)dwritef.GetAddressOf());
            HR_VFY(hr);

            HRESULT hr = m_2ddc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &brush);
            HR_VFY(hr);
            hr = dwritef->CreateTextFormat(
                L"Robot Radicals",
                NULL,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                40,
                L"en-us",
                &format
            );
            HR_VFY(hr);
            hr = format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            HR_VFY(hr);
            hr = format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            HR_VFY(hr);
        }
    }
}

DOG::gfx::d2dBackend_DX12::~d2dBackend_DX12()
{

}

void DOG::gfx::d2dBackend_DX12::BeginFrame(RenderDevice* rd, Swapchain* sc)
{
    u_char idx = sc->GetNextDrawSurfaceIdx();
    D2D1_SIZE_F rtSize = m_d2dRenderTargets[idx]->GetSize();
    

    m_11on12d->AcquireWrappedResources(m_wrappedBackBuffers[idx].GetAddressOf(), 1);

    m_2ddc->SetTarget(m_d2dRenderTargets[idx].Get());
    m_2ddc->BeginDraw();
}

void DOG::gfx::d2dBackend_DX12::Render(RenderDevice* rd, Swapchain* sc)
{
    
}

void DOG::gfx::d2dBackend_DX12::EndFrame(RenderDevice* rd, Swapchain* sc)
{
    u_char idx = sc->GetNextDrawSurfaceIdx();
    HRESULT hr = m_2ddc->EndDraw();
    HR_VFY(hr);

    m_11on12d->ReleaseWrappedResources(m_wrappedBackBuffers[idx].GetAddressOf(), 1);

    m_dc->Flush();
}