#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl/client.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#define HR_VFY(hr) assert(SUCCEEDED(hr))

// Will leak if GPU Validation On (changed)
// --> If on: Will crash in the GPU Validation Layer at some point for an unknown reason
#define GPU_VALIDATION_ON


