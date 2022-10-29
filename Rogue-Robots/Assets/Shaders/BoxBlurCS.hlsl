#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_Samplers.hlsli"

struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
    
    uint renderWidth;
    uint renderHeight;
    uint input;
    uint output;
    
    uint isHorizontal;
    uint downscaleFactor;
};
CONSTANTS(g_constants, PushConstantElement)


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

static const float gFilter[7] = { 0.071303, 0.131514, 0.189879, 0.214607, 0.189879, 0.131514, 0.071303 };


[numthreads(8, 8, 1)]
void main(uint3 globalId : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID)
{
    if (globalId.x > g_constants.renderWidth || globalId.y > g_constants.renderHeight)
        return;
    
    float2 uv = float2((float) globalId.x / g_constants.renderWidth, (float) globalId.y / g_constants.renderHeight);
    
    StructuredBuffer<ShaderInterop_GlobalData> gds = ResourceDescriptorHeap[g_constants.gdDescriptor];
    ShaderInterop_GlobalData gd = gds[0];
    
    StructuredBuffer<ShaderInterop_PerFrameData> pfDatas = ResourceDescriptorHeap[gd.perFrameTable];
    ShaderInterop_PerFrameData pfData = pfDatas[g_constants.perFrameOffset];
    
    // Grab resources
    Texture2D<float4> input = ResourceDescriptorHeap[g_constants.input];
    RWTexture2D<float4> output = ResourceDescriptorHeap[g_constants.output];
    
    //output[globalId.xy] = input.Load(globalId);
    //return;
    
    
    int size = 3;
    
    float4 accum = 0.f;

    if (g_constants.isHorizontal == 1)
    {
        for (int i = -size; i <= size; ++i)
        {
            accum += gFilter[i + size] * input.Load(uint3(globalId.xy * g_constants.downscaleFactor + uint2(i, 0), 0));
        }
    }
    else
    {
        for (int i = -size; i <= size; ++i)
        {
            accum += gFilter[i + size] * input.Load(uint3(globalId.xy * g_constants.downscaleFactor + uint2(0, i), 0));
        }
    }
    
    
    
    //accum /= pow(size * 2 + 1, 2);
    
    output[globalId.xy] *= accum;
  
}