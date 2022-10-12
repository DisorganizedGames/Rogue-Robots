#include "ShaderInterop_Samplers.hlsli"

struct PushConstantElement
{
    uint srcTexture;
    uint dstTexture;
    uint width;
    uint height;
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);


static const float filter[7][7] =
{
    0.0166297, 0.0183786, 0.0195151, 0.0199093, 0.0195151, 0.0183786, 0.0166297,
    0.0183786, 0.0203115, 0.0215675, 0.0220032, 0.0215675, 0.0203115, 0.0183786,
    0.0195151, 0.0215675, 0.0229012, 0.0233638, 0.0229012, 0.0215675, 0.0195151,
    0.0199093, 0.0220032, 0.0233638, 0.0238358, 0.0233638, 0.0220032, 0.0199093,
    0.0195151, 0.0215675, 0.0229012, 0.0233638, 0.0229012, 0.0215675, 0.0195151,
    0.0183786, 0.0203115, 0.0215675, 0.0220032, 0.0215675, 0.0203115, 0.0183786,
    0.0166297, 0.0183786, 0.0195151, 0.0199093, 0.0195151, 0.0183786, 0.0166297
};


[numthreads(32, 32, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    if (globalId.x < g_constants.width && globalId.y < g_constants.height)
    {
        Texture2D bloomSrcTexture = ResourceDescriptorHeap[g_constants.srcTexture];
        RWTexture2D<float4> bloomDstTexture = ResourceDescriptorHeap[g_constants.dstTexture];
        
        float pixelWidth = 1.0f / g_constants.width;
        float pixelHeight = 1.0f / g_constants.height;

        //bloomDstTexture[globalId.xy].rgb = bloomSrcTexture.Sample(g_point_clamp_samp, float2(globalId.x * pixelWidth, globalId.y * pixelHeight)).rgb;
        //return;
        
        
        // Swap to have y in the outer loop might give better performance, but this pass should be replaced with a two stage blure pass if performance is a problem
        float3 color = float3(0, 0, 0);
        for (int x = 0; x < 7; x++)
        {
            float u = pixelWidth * (globalId.x - 3 + x);
            if (u < 0.0f || u > 1.0f) continue;
            for (int y = 0; y < 7; y++)
            {
                float v = pixelHeight * (globalId.y - 3 + y);
                if (v < 0.0f || v > 1.0f) continue;
                color += filter[x][y] * bloomSrcTexture.Sample(g_point_clamp_samp, float2(u, v)).rgb;
            }
        }
        bloomDstTexture[globalId.xy].rgb = color;
    }
}