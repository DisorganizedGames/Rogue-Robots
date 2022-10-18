#include "D2DBackend_DX12.h"


#include "RenderDevice_DX12.h"
#include "Swapchain_DX12.h"


DOG::gfx::D2DBackend_DX12::D2DBackend_DX12(RenderDevice* rd, Swapchain* sc, u_int numBuffers) : m_rd(rd), m_sc(sc), m_numBuffers(numBuffers)
{
    // This is guaranteed since we have no other backends than DX12
    auto rd12 = (RenderDevice_DX12*)rd;
    auto sc12 = (Swapchain_DX12*)sc;

    m_renderTargets.resize(numBuffers);
    m_wrappedBackBuffers.resize(numBuffers);
    m_d2dRenderTargets.resize(numBuffers);

    // Create an 11 device wrapped around the 12 device and share
    // 12's command queue.
    auto queue = rd12->GetQueue();
    HRESULT hr = D3D11On12CreateDevice(
        rd12->GetDevice(),
        D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        NULL,
        1u,
        reinterpret_cast<IUnknown**>(&queue),
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
        D2D1_FACTORY_OPTIONS fOpt = {
            .debugLevel = D2D1_DEBUG_LEVEL_INFORMATION
        };
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory5), &fOpt, (void**)&m_factory);
        HR_VFY(hr);
        hr = m_11on12d.As(&m_dxd);
        HR_VFY(hr);
        hr = m_factory->CreateDevice(m_dxd.Get(), m_2dd.GetAddressOf()); //warning
        HR_VFY(hr);
        hr = m_2dd->CreateDeviceContext(deviceOptions, &m_2ddc);
        HR_VFY(hr);
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&m_dwritwf);
        HR_VFY(hr);
    }


    //float dpi = (float)GetDpiForWindow(hwnd);
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

    {

        // Create a RTV, D2D render target, and a command allocator for each frame.
        for (u8 n = 0; n < m_numBuffers; n++)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = rd12->GetReservedRTV(n);
            m_renderTargets[n] = sc12->GetD12Buffer((u8)n);
            rd12->GetDevice()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtv);

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

            //Create a render target for D2D to draw directly to this back buffer.

            hr = m_wrappedBackBuffers[n].As(&surface);
            HR_VFY(hr);
            hr = m_2ddc->CreateBitmapFromDxgiSurface(
                surface.Get(),
                bitmapProperties,
                m_d2dRenderTargets[n].GetAddressOf()
            );
            HR_VFY(hr);
        }

            hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)this->m_dwritwf.GetAddressOf());
            HR_VFY(hr);
    }
}

void DOG::gfx::D2DBackend_DX12::OnResize()
{
    auto rd12 = (RenderDevice_DX12*)m_rd;
    auto sc12 = (Swapchain_DX12*)m_sc;

    auto queue = rd12->GetQueue();
    HRESULT hr = D3D11On12CreateDevice(
        rd12->GetDevice(),
        D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        NULL,
        1u,
        reinterpret_cast<IUnknown**>(&queue),
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
        D2D1_FACTORY_OPTIONS fOpt = {
            .debugLevel = D2D1_DEBUG_LEVEL_INFORMATION
        };
        //hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory5), &fOpt, (void**)&m_factory);
        HR_VFY(hr);
        hr = m_11on12d.As(&m_dxd);
        HR_VFY(hr);
        hr = m_factory->CreateDevice(m_dxd.Get(), m_2dd.GetAddressOf()); //warning
        HR_VFY(hr);
        hr = m_2dd->CreateDeviceContext(deviceOptions, &m_2ddc);
        HR_VFY(hr);
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&m_dwritwf);
        HR_VFY(hr);
    }

    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
    for (u8 n = 0; n < m_numBuffers; n++)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = rd12->GetReservedRTV(n);
        m_renderTargets[n] = sc12->GetD12Buffer((u8)n);
        
        rd12->GetDevice()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtv);
        D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
        hr = m_11on12d->CreateWrappedResource(
            m_renderTargets[n].Get(),
            &d3d11Flags,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT,
            IID_PPV_ARGS(&m_wrappedBackBuffers[n])
        );
        hr = m_wrappedBackBuffers[n].As(&surface);
        HR_VFY(hr);
        hr = m_2ddc->CreateBitmapFromDxgiSurface(
            surface.Get(),
            bitmapProperties,
            m_d2dRenderTargets[n].GetAddressOf()
        );
        HR_VFY(hr);
        
    }
}

void DOG::gfx::D2DBackend_DX12::FreeResize()
{
    surface.Reset();
    for (size_t i = 0; i < m_numBuffers; i++)
    {
        m_renderTargets[i].Reset();
        m_d2dRenderTargets[i].Reset();
        m_wrappedBackBuffers[i].Reset();
    }
    m_d.Reset();
    m_2dd.Reset();
    m_11on12d.Reset();
    m_dc.Reset();
    m_2ddc.Reset();
    m_dxd.Reset();
    //m_dwritwf.Reset();
    //m_factory.Reset();
}

DOG::gfx::D2DBackend_DX12::~D2DBackend_DX12()
{

}

void DOG::gfx::D2DBackend_DX12::BeginFrame()
{
    u_char idx = m_sc->GetNextDrawSurfaceIdx();
    D2D1_SIZE_U rtSize = m_d2dRenderTargets[idx]->GetPixelSize();


    m_11on12d->AcquireWrappedResources(m_wrappedBackBuffers[idx].GetAddressOf(), 1);

    m_2ddc->SetTarget(m_d2dRenderTargets[idx].Get());
    m_2ddc->BeginDraw();
}

void DOG::gfx::D2DBackend_DX12::Render()
{

}

void DOG::gfx::D2DBackend_DX12::EndFrame()
{
    u_char idx = m_sc->GetNextDrawSurfaceIdx();
    HRESULT hr = m_2ddc->EndDraw(); //warning
    HR_VFY(hr);

    m_11on12d->ReleaseWrappedResources(m_wrappedBackBuffers[idx].GetAddressOf(), 1);

    m_dc->Flush();
}