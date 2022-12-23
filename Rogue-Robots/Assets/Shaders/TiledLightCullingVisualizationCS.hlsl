#include "ShaderInterop_Lights.hlsli"
#include "ShaderInterop_Renderer.h"
struct PushConstantElement
{
    uint hdrTexture;
    uint localLightBuffersIndex;
    uint width;
    uint height;
};
ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);


//#define LOCAL_LIGHT_MAX_SIZE 31

//struct LocalLightBuffer
//{
//    uint count;
//    uint lightIndices[LOCAL_LIGHT_MAX_SIZE];
//};

[numthreads(TILED_GROUP_DIM, TILED_GROUP_DIM, 1)]
void main(uint3 globalId : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    uint tid = threadId.x + TILED_GROUP_DIM * threadId.y;
    uint tileIndex = groupID.x + (g_constants.width + TILED_GROUP_DIM - 1) / TILED_GROUP_DIM * groupID.y;
    if (globalId.x < g_constants.width && globalId.y < g_constants.height)
    {   
        StructuredBuffer<ShaderInterop_LocalLightBuffer> localLightBuffers = ResourceDescriptorHeap[g_constants.localLightBuffersIndex];
        RWTexture2D<float4> dst = ResourceDescriptorHeap[g_constants.hdrTexture];
        
        uint count = localLightBuffers[tileIndex].count;
        if (count == 1)
        {
            dst[globalId.xy].rgb = float3(0, 0, 1);
        }
        else if (count == 2)
        {
            dst[globalId.xy].rgb = float3(1, 1, 0);
        }
        else if (count > 2)
        {
            dst[globalId.xy].rgb = float3(1, 0, 0);
        }
    }
}