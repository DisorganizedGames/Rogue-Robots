#include "ShaderInterop_Samplers.hlsli"

struct PushConstantElement
{
    uint srcTexture;
    uint dstTexture;
    uint width;
    uint height;
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

[numthreads(32, 32, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    if (globalId.x < g_constants.width && globalId.y < g_constants.height)
    {
        Texture2D bloomSrcTexture = ResourceDescriptorHeap[g_constants.srcTexture];
        RWTexture2D<float4> bloomDstTexture = ResourceDescriptorHeap[g_constants.dstTexture];

        float pixelWidth = 1.0f / g_constants.width;
        float pixelHeight = 1.0f / g_constants.height;

        bloomDstTexture[globalId.xy].rgb += bloomSrcTexture.Sample(g_bilinear_clamp_samp, float2(globalId.x * pixelWidth, globalId.y * pixelHeight)).rgb;
    }
}