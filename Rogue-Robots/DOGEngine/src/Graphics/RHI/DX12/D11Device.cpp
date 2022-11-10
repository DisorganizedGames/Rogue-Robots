#include "D11Device.h"
#include "Utilities/DX12Queue.h"

static ID3D11Device* s_d11dev = nullptr;
static DX12Queue* s_gfxQueue = nullptr;

void InitializeD11(ID3D12Device* dev, DX12Queue* queue)
{
	HRESULT hr{ S_OK };

	hr = D3D11On12CreateDevice(
		dev,
		D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		NULL,
		1u,
		reinterpret_cast<IUnknown**>(&(*queue)),
		1,
		0,
		&s_d11dev,
		nullptr,
		nullptr
	);
	assert(SUCCEEDED(hr));
	s_gfxQueue = queue;
}

void DestroyD11()
{
	s_d11dev->Release();
}

ID3D11Device* GetD11Device()
{
	return s_d11dev;
}

void D11Flush()
{
	s_gfxQueue->flush();
}

void D11ReInit(ID3D12Device* dev)
{
	HRESULT hr = D3D11On12CreateDevice(
		dev,
		D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		NULL,
		1u,
		reinterpret_cast<IUnknown**>(&(*s_gfxQueue)),
		1,
		0,
		&s_d11dev,
		nullptr,
		nullptr
	);
	assert(SUCCEEDED(hr));
}


