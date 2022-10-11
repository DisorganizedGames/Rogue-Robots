#include "ShaderInterop_Samplers.hlsli"

struct PushConstantElement
{
    uint srcTexture;
    uint dstTexture;
    uint contantBufferHandle;
    uint width;
    uint height;
};

struct PerDrawData
{
    float3 color;
    float threshold;
    uint2 res;
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);


[numthreads(32, 32, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    if (globalId.x < g_constants.width && globalId.y < g_constants.height)
    {
        RWTexture2D<float4> bloomSrcTexture = ResourceDescriptorHeap[g_constants.srcTexture];
        RWTexture2D<float4> bloomDstTexture = ResourceDescriptorHeap[g_constants.dstTexture];
        
        bloomDstTexture[globalId.xy].rgb = bloomSrcTexture[globalId.xy].rgb;
    }
}