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

[numthreads(32, 32, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    if (globalId.x < g_constants.width && globalId.y < g_constants.height)
    {
        ConstantBuffer<PerDrawData> cb = ResourceDescriptorHeap[g_constants.contantBufferHandle];

        Texture2D srcTexture = ResourceDescriptorHeap[g_constants.srcTexture];
        RWTexture2D<float4> bloomTexture = ResourceDescriptorHeap[g_constants.dstTexture];

        float u = (float) globalId.x / g_constants.width;
        float v = (float) globalId.y / g_constants.height;
        float3 color = srcTexture.Sample(g_bilinear_clamp_samp, float2(u, v)).rgb;

        if (length(color.rgb) > length(cb.threshold * float3(1, 1, 1)))
        {
            bloomTexture[globalId.xy].rgb = color;
        }
    }
}



//float3 random(float seed, float3 seedVec)
//{
//    float r1 = frac(sin(dot(float2(seed, seedVec.x), float2(12.9898, 78.233))) * 43758.5453123);
//    float r2 = frac(sin(dot(float2(r1, seedVec.y), float2(12.9898, 78.233))) * 43758.5453123);
//    float r3 = frac(sin(dot(float2(r2, seedVec.z), float2(12.9898, 78.233))) * 43758.5453123);
//    return float3(r1, r2, r3);
//}

//[numthreads(32, 32, 1)]
//void main(uint3 globalId : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID, uint3 groupID : SV_GroupID)
//{
//    ConstantBuffer<PerDrawData> cb = ResourceDescriptorHeap[g_constants.contantBufferHandle];
//    if (globalId.x < cb.res.x && globalId.y < cb.res.y)
//    {
//        RWTexture2D<float4> tex = ResourceDescriptorHeap[g_constants.srcTexture];
//        RWTexture2D<float4> bloomTexture = ResourceDescriptorHeap[g_constants.dstTexture];
//        float3 c = random(groupID.x + 32 * groupID.y, cb.color);
//        if (length(tex[globalId.xy].rgb) < length(cb.threshold * float3(1, 1, 1)))
//        {
//            tex[globalId.xy] = float4(c, 1.f);
//            bloomTexture[globalId.xy] = float4(c, 1.f);
//        }
//    }
//}