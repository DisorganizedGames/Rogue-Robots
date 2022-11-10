#include "ShaderInterop_Lights.hlsli"
#include "ShaderInterop_Renderer.h"
struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
    uint hdrTexture;
    uint constantBufferIndex;
    uint width;
    uint height;
};
ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);


#define LOCAL_LIGHT_MAX_SIZE 32
groupshared uint sLightCounter;
groupshared uint sLocalLightList[LOCAL_LIGHT_MAX_SIZE];


#define groupSize 16
[numthreads(groupSize, groupSize, 1)]
void main(uint3 globalId : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    uint tid = threadId.x + 16 * threadId.y;
    if (tid == 0)
    {
        sLightCounter = 0;
    }
    GroupMemoryBarrierWithGroupSync();


    if (globalId.x < g_constants.width && globalId.y < g_constants.height)
    {
        StructuredBuffer<ShaderInterop_GlobalData> gds = ResourceDescriptorHeap[g_constants.gdDescriptor];
        ShaderInterop_GlobalData gd = gds[0];
        
        StructuredBuffer<ShaderInterop_LightsMetadata> lightsMDs = ResourceDescriptorHeap[gd.lightTableMD];
        ShaderInterop_LightsMetadata lightsMD = lightsMDs[0];
        
        StructuredBuffer<ShaderInterop_PerFrameData> pfDatas = ResourceDescriptorHeap[gd.perFrameTable];
        ShaderInterop_PerFrameData pfData = pfDatas[g_constants.perFrameOffset];
        
        
        float2 scale = float2(g_constants.width, g_constants.height) * rcp(2.0f * groupSize);
        float2 bias = scale - float2(groupID.xy);
        
        float p11 = pfData.projMatrix._11;
        float p22 = pfData.projMatrix._22;
        
        //float4 col1 = float4(p11 * scale.x * 2, 0, 2 * bias.x - 1, 0);
        //float4 col2 = float4(0, -p22 * scale.y * 2, 2 * bias.y - 1, 0);
        
        float4 col1 = float4(p11 * scale.x, 0, bias.x, 0);
        float4 col2 = float4(0, -p22 * scale.y, bias.y, 0);
        
        float4 col4 = float4(0, 0, 1, 0);
        float4 frustum[6];
        
        frustum[0] = col4 + col1;
        frustum[1] = col4 - col1;
        frustum[2] = col4 + col2;
        frustum[3] = col4 - col2;
        frustum[4] = float4(0, 0, 1, -0.1f);
        frustum[5] = float4(0, 0, -1, 100.0f);
        
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


        StructuredBuffer<ShaderInterop_PointLight> pointLights = ResourceDescriptorHeap[gd.pointLightTable];
        for (int i = tid; i < lightsMD.dynPointLightRange.count; i += groupSize * groupSize)
        {
            uint globalIndex = pfData.pointLightOffsets.dynOffset + i;
            ShaderInterop_PointLight pointLight = pointLights[globalIndex];
            bool culled = false;

            for (int j = 0; j < 6; j++)
            {
                float d = dot(float4(pointLight.position.xyz, 1), frustum[j]);
                culled |= d * abs(d) < -3.0f * pointLight.strength;
            }

            if (!culled && pointLight.strength)
            {
                uint locallIndex;
                
                InterlockedAdd(sLightCounter, 1, locallIndex);
                sLocalLightList[locallIndex] = globalIndex;
            }
        }
        //GroupMemoryBarrierWithGroupSync();
        
        //RWTexture2D<float4> dst = ResourceDescriptorHeap[g_constants.hdrTexture];
    
        //if (sLightCounter == 1)
        //{
        //    dst[globalId.xy].rgb = float3(0, 0 ,1);
        //}
        //else if (sLightCounter == 2)
        //{
        //    dst[globalId.xy].rgb = float3(1, 1, 0);
        //}
        //else if (sLightCounter > 2)
        //{
        //    dst[globalId.xy].rgb = float3(1,0,0);
        //}
    }
}