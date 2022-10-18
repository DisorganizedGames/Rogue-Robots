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
void main(uint3 globalId : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    if (globalId.x < g_constants.width && globalId.y < g_constants.height)
    {
        Texture2D srcTexture = ResourceDescriptorHeap[g_constants.srcTexture];
        RWTexture2D<float4> target = ResourceDescriptorHeap[g_constants.dstTexture];

        float u = (float) globalId.x / g_constants.width;
        float v = (float) globalId.y / g_constants.height;

        float3 color = srcTexture.Sample(g_bilinear_clamp_samp, float2(u, v)).rgb;
        target[globalId.xy].xyz = color;
    }
}
