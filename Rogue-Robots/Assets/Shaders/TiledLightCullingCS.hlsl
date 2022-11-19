#include "ShaderInterop_Lights.hlsli"
#include "ShaderInterop_Renderer.h"
struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
    uint localLightBuffersIndex;
    uint depthBufferIndex;
    uint width;
    uint height;
    float pointLightCullFactor;
};
ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);


groupshared uint sLightCounter;
groupshared uint sharedUintMinZ;
groupshared uint sharedUintMaxZ;

float ConvertDepthToViewSpace(matrix projInv, float depth)
{
    float4 viewMin = float4(0, 0, depth, 1.0f);
    viewMin = mul(projInv, viewMin);
    depth = viewMin.z / viewMin.w;
    return depth;
}

[numthreads(TILED_GROUP_SIZE, TILED_GROUP_SIZE, 1)]
void main(uint3 globalId : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID, uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint tid = threadId.x + TILED_GROUP_SIZE * threadId.y;
    if (tid == 0)
    {
        sLightCounter = 0;
        sharedUintMaxZ = 0;
        sharedUintMinZ = 0x7f7fffff;
    }
    GroupMemoryBarrierWithGroupSync();
    
    StructuredBuffer<ShaderInterop_GlobalData> gds = ResourceDescriptorHeap[g_constants.gdDescriptor];
    ShaderInterop_GlobalData gd = gds[0];
    
    StructuredBuffer<ShaderInterop_PerFrameData> pfDatas = ResourceDescriptorHeap[gd.perFrameTable];
    ShaderInterop_PerFrameData pfData = pfDatas[g_constants.perFrameOffset];
    
    Texture2D depthBuffer = ResourceDescriptorHeap[g_constants.depthBufferIndex];
    float depth = depthBuffer[globalId.xy].r;
    
    uint depthUint = asuint(ConvertDepthToViewSpace(pfData.projMatrix, depth));
    if (depth != 0.0f)
    {
        InterlockedMin(sharedUintMinZ, depthUint);
        InterlockedMax(sharedUintMaxZ, depthUint);
    }

    GroupMemoryBarrierWithGroupSync();
    RWStructuredBuffer<ShaderInterop_LocalLightBuffer> localLightBuffers = ResourceDescriptorHeap[g_constants.localLightBuffersIndex];
    uint tileIndex = groupID.x + (g_constants.width + TILED_GROUP_SIZE - 1) / TILED_GROUP_SIZE * groupID.y;
    StructuredBuffer<ShaderInterop_LightsMetadata> lightsMDs = ResourceDescriptorHeap[gd.lightTableMD];
    ShaderInterop_LightsMetadata lightsMD = lightsMDs[0];

    uint numTilesX = (g_constants.width + TILED_GROUP_SIZE - 1) / TILED_GROUP_SIZE;
    uint numTilesY = (g_constants.height + TILED_GROUP_SIZE - 1) / TILED_GROUP_SIZE;
    uint width = numTilesX * TILED_GROUP_SIZE;
    uint height = numTilesY * TILED_GROUP_SIZE;
    float2 scale = float2(width, height) * rcp(2.0f * TILED_GROUP_SIZE);
    float2 bias = scale - float2(groupID.xy);
    
    float p11 = pfData.projMatrix._11;
    float p22 = pfData.projMatrix._22;
    
    float4 col1 = float4(p11 * scale.x * 2, 0, 2 * bias.x - 1, 0);
    float4 col2 = float4(0, -p22 * scale.y * 2, 2 * bias.y - 1, 0);
    
    //float4 col1 = float4(p11 * scale.x, 0, bias.x, 0);
    //float4 col2 = float4(0, -p22 * scale.y, bias.y, 0);
    
    float4 col4 = float4(0, 0, 1, 0);
    float4 frustum[6];
    
    frustum[0] = col4 + col1;
    frustum[1] = col4 - col1;
    frustum[2] = col4 + col2;
    frustum[3] = col4 - col2;

    bool useDepth = true; // Add option to toggle later
    if (useDepth)
    {
        float near = asfloat(sharedUintMinZ);
        float far = asfloat(sharedUintMaxZ);
        
        frustum[4] = float4(0, 0, 1, -near);
        frustum[5] = float4(0, 0, -1, far);
    }
    else
    {
        frustum[4] = float4(0, 0, 1, -pfData.nearClip);
        frustum[5] = float4(0, 0, -1, pfData.farClip);
    }


    frustum[0] *= rcp(length(frustum[0].xyz));
    frustum[1] *= rcp(length(frustum[1].xyz));
    frustum[2] *= rcp(length(frustum[2].xyz));
    frustum[3] *= rcp(length(frustum[3].xyz));
    
    matrix viewT = transpose(pfData.viewMatrix);
    
    
    frustum[0] = mul(viewT, frustum[0]);
    frustum[1] = mul(viewT, frustum[1]);
    frustum[2] = mul(viewT, frustum[2]);
    frustum[3] = mul(viewT, frustum[3]);
    frustum[4] = mul(viewT, frustum[4]);
    frustum[5] = mul(viewT, frustum[5]);

    
    StructuredBuffer<ShaderInterop_PointLight> pointLights = ResourceDescriptorHeap[gd.pointLightTable]; // fix a new buffer that only holds position and radius to be more cache friendly
    float plCullFactor = g_constants.pointLightCullFactor;
   
    
    for (int i = tid; i < lightsMD.dynPointLightRange.count; i += TILED_GROUP_SIZE * TILED_GROUP_SIZE)
    {
        uint globalIndex = pfData.pointLightOffsets.dynOffset + i;
        ShaderInterop_PointLight pointLight = pointLights[globalIndex];
        bool culled = false;

        for (int j = 0; j < 6; j++)
        {
            float d = dot(float4(pointLight.position.xyz, 1), frustum[j]);
            culled |= d < -pointLight.radius;
        }

        if (!culled && pointLight.strength)
        {
            uint locallIndex;
            InterlockedAdd(sLightCounter, 1, locallIndex);
            localLightBuffers[tileIndex].lightIndices[locallIndex] = globalIndex;
        }
    }
    
    
    for (int i = tid; i < lightsMD.staticPointLightRange.count; i += TILED_GROUP_SIZE * TILED_GROUP_SIZE)
    {
        uint globalIndex = pfData.pointLightOffsets.staticOffset + i;
        ShaderInterop_PointLight pointLight = pointLights[globalIndex];
        bool culled = false;
        for (int j = 0; j < 6; j++)
        {
            float d = dot(float4(pointLight.position.xyz, 1), frustum[j]);
            culled |= d < -pointLight.radius;
        }

        if (!culled && pointLight.strength)
        {
            uint locallIndex;
            InterlockedAdd(sLightCounter, 1, locallIndex);
            localLightBuffers[tileIndex].lightIndices[locallIndex] = globalIndex;
        }
        
    }
    GroupMemoryBarrierWithGroupSync();

    if (tid == 0)
    {
        localLightBuffers[tileIndex].count = sLightCounter;
    }
}