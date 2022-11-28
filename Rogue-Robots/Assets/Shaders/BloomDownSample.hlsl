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
    0.000840725, 0.00301024, 0.00647097, 0.00835139, 0.00647097, 0.00301024, 0.000840725,
    0.00301024,  0.0107783,  0.0231695,  0.0299024,  0.0231695,  0.0107783,  0.00301024,
    0.00647097,  0.0231695,  0.0498063,  0.0642797,  0.0498063,  0.0231695,  0.00647097,
    0.00835139,  0.0299024,  0.0642797,  0.082959,   0.0642797,  0.0299024,  0.00835139,
    0.00647097,  0.0231695,  0.0498063,  0.0642797,  0.0498063,  0.0231695,  0.00647097,
    0.00301024,  0.0107783,  0.0231695,  0.0299024,  0.0231695,  0.0107783,  0.00301024,
    0.000840725, 0.00301024, 0.00647097, 0.00835139, 0.00647097, 0.00301024, 0.000840725
};

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

        // Swap to have y in the outer loop might give better performance, but this pass should be replaced with a two stage blur pass if performance is a problem
        float3 color = float3(0, 0, 0);
        for (int x = 0; x < 7; x++)
        {
            //float u = pixelWidth * (globalId.x + 0.5f - 3 + x);
            float u = pixelWidth * (globalId.x + x - 2.5f); // -2.5 pixels. +0.5: We downscale to half resolution gives. -3: The filter is 7 pixels wide and and -3 is the start offset
            if (u < 0.0f || u > 1.0f) continue;
            for (int y = 0; y < 7; y++)
            {
                float v = pixelHeight * (globalId.y + y - 2.5f);
                if (v < 0.0f || v > 1.0f) continue;
                color += filter[x][y] * bloomSrcTexture.Sample(g_bilinear_clamp_samp, float2(u, v)).rgb;
                //color += filter[x][y] * bloomSrcTexture.Sample(g_point_clamp_samp, float2(u, v)).rgb;
            }
        }
        bloomDstTexture[globalId.xy].rgb = color;
    }
}