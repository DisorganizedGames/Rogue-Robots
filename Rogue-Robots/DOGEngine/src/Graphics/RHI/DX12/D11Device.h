#pragma once
#include <d3d11on12.h>
#include <assert.h>

class DX12Queue;

void InitializeD11(ID3D12Device* dev, DX12Queue* queue);
void DestroyD11();
ID3D11Device* GetD11Device();
void D11Flush();
void D11ReInit(ID3D12Device* dev);

