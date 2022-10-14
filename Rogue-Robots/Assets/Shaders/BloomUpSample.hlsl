#include "ShaderInterop_Samplers.hlsli"

struct PushConstantElement
{
    uint srcTexture;
    uint dstTexture;
    uint width;
    uint height;
};

static const float filter[3][3] =
{
    1, 2, 1,
    2, 4, 2,
    1, 2, 1,
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);
#define groupSize 32
[numthreads(groupSize, groupSize, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    if (globalId.x < g_constants.width && globalId.y < g_constants.height)
    {
        Texture2D bloomSrcTexture = ResourceDescriptorHeap[g_constants.srcTexture];
        RWTexture2D<float4> bloomDstTexture = ResourceDescriptorHeap[g_constants.dstTexture];

        float pixelWidth = 1.0f / g_constants.width;
        float pixelHeight = 1.0f / g_constants.height;
        float f = 1.0f / 16.0f;
        float3 color = float3(0, 0, 0);
        for (int x = 0; x < 3; x++)
        {
            float u = pixelWidth * (globalId.x + x - 0.5f);
            if (u < 0.0f || u > 1.0f) continue;
            for (int y = 0; y < 3; y++)
            {
                float v = pixelHeight * (globalId.y + y - 0.5f);
                if (v < 0.0f || v > 1.0f) continue;
                color += f * filter[x][y] * bloomSrcTexture.Sample(g_bilinear_clamp_samp, float2(u, v)).rgb;
            }
        }
        bloomDstTexture[globalId.xy].rgb += color;
    }
}