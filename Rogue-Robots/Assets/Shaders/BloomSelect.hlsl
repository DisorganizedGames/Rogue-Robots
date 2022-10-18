#include "ShaderInterop_Samplers.hlsli"

struct PushConstantElement
{
    uint srcTexture;
    uint dstTexture;
    uint width;
    uint height;
    uint contantBufferHandle;
};

struct PerDrawData
{
    float threshold;
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

#define groupSize 32
[numthreads(groupSize, groupSize, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    if (globalId.x < g_constants.width && globalId.y < g_constants.height)
    {
        ConstantBuffer<PerDrawData> cb = ResourceDescriptorHeap[g_constants.contantBufferHandle];

        Texture2D srcTexture = ResourceDescriptorHeap[g_constants.srcTexture];
        RWTexture2D<float4> bloomTexture = ResourceDescriptorHeap[g_constants.dstTexture];

        float u = (float) (globalId.x + 0.5f) / g_constants.width;
        float v = (float) (globalId.y + 0.5f) / g_constants.height;
        float3 color = srcTexture.Sample(g_bilinear_clamp_samp, float2(u, v)).rgb;

        if (length(color.rgb) > length(cb.threshold * float3(1, 1, 1)))
        {
            float luma = dot(float3(0.2126f, 0.7152f, 0.0722), color);
            float weight = 1.0f / (1.0f + luma);
            bloomTexture[globalId.xy].rgb = color * weight;
        }
    }
}
