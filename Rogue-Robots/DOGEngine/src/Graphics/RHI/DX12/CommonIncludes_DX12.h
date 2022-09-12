#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl/client.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#define HR_VFY(hr) assert(SUCCEEDED(hr))


