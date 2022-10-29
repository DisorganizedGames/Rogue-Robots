#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_Samplers.hlsli"
#include "AcesTonemapping.hlsli"

struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
    
    uint renderWidth;
    uint renderHeight;
    uint aoOut;
    uint depth;
    uint normal;
    uint noise;
    uint samples;
};
ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);


float3 VSPositionFromDepth(float2 vTexCoord, float z, matrix invProj)
{
    // Get the depth value for this pixel
    // z
    
    // Get x/w and y/w from the viewport position
    float x = vTexCoord.x * 2.f - 1.f;
    float y = (1.f - vTexCoord.y) * 2.f - 1.f;
    float4 vProjectedPos = float4(x, y, z, 1.0f);
    // Transform by the inverse projection matrix
    float4 vPositionVS = mul(invProj, vProjectedPos);
    //float4 vPositionVS = mul(vProjectedPos, invProj);
    // Divide by w to get the view-space position
    return vPositionVS.xyz / vPositionVS.w;
}

[numthreads(32, 32, 1)]
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
    RWTexture2D<float4> aoOut = ResourceDescriptorHeap[g_constants.aoOut];
    Texture2D<float> depths = ResourceDescriptorHeap[g_constants.depth];
    Texture2D normals = ResourceDescriptorHeap[g_constants.normal];
    Texture2D noise = ResourceDescriptorHeap[g_constants.noise];
    StructuredBuffer<float4> samples = ResourceDescriptorHeap[g_constants.samples];
    
    float depth = depths.Load(uint3(globalId.xy, 0));
    float3 wsNormal = normalize(normals.Load(uint3(globalId.xy, 0.f)).xyz);
    float3 vsNormal = normalize(mul(pfData.viewMatrix, float4(wsNormal, 0.f)).xyz);
    float3 vsPos = VSPositionFromDepth(uv, depth, pfData.invProjMatrix);
    float3 wsPos = mul(pfData.invViewMatrix, float4(vsPos, 1.f));
    
    // Grab noise texture
    float2 noiseScale = float2(g_constants.renderWidth / 4.f, g_constants.renderHeight / 4.f);
    float3 randVec = noise.Sample(g_point_samp, uv * noiseScale).xyz;
    randVec = normalize(randVec * 2.f - 1.f.rrr); // Unpack [0, 1] --> [-1, 1]
        
    // Construct TBN matrix
    float3 nor = normalize(vsNormal);
    float3 tan = normalize(randVec - nor * dot(nor, randVec));  // Gram-Schmidt process
    float3 bitan = normalize(cross(nor, tan));                  // Assuming RH space
    float3x3 tbn = float3x3(tan, bitan, nor);                   // Z is up in tangent space
                
    //float radius = 0.4f;
    float radius = 0.7;
    float occludedRatio = 0.f;
    for (int i = 0; i < 16; ++i)
    {
        float3 vsSampleInHemi = vsPos + mul(tbn, samples[i].xyz) * radius;
        float4 ndc = mul(pfData.projMatrix, float4(vsSampleInHemi, 1.f));
        ndc.xyz /= ndc.w; // now in [-1, 1] in xy with (0,0) as screen origin
        
        // Not inside NDC --> No contrib
        //if (!(ndc.x > -1.f && ndc.x < 1.f || ndc.y < 1.f || ndc.y > -1.f))
        //{
        //    occludedRatio += 1.f;
        //    continue;
        //}
        
        // Scale so that [0, 0] is top-left and stretches [0, 1] in xy
        ndc.y *= -1.f;
        ndc.xy = ndc.xy * 0.5f + 0.5f.rr; // --> Works as UV to sample depth with
        
        float sampleDepthCS = depths.Sample(g_point_samp, ndc.xy);
        //float3 sampleVsPos = VSPositionFromDepth(ndc.xy, sampleDepthCS, pfData.invProjMatrix);
        //float sampleDepth = sampleVsPos.z;
        
        float rangeCheck = abs(ndc.z - sampleDepthCS) < (0.01) ? 1.0 : 0.0;
        //float rangeCheck = smoothstep(0.0, 1.0, radius / abs(vsSampleInHemi.z - sampleDepth));
        
        //occludedRatio += (sampleDepth < vsSampleInHemi.z ? 1.0 : 0.0) * rangeCheck;
        occludedRatio += (sampleDepthCS >= ndc.z ? 1.0 : 0.0) * rangeCheck;
        
    }
    occludedRatio /= 16.f;
    float contrib = 1.f - occludedRatio;

    //float3 final = reinhard_jodie(contrib.rrr * 7.f).rrr;
    float3 final = uncharted2_filmic(contrib.rrr * 5.f).rrr;
    //float3 final = contrib.rrr;
    
    // TEMP?
    final *= final;
    
    
    aoOut[globalId.xy] = float4(final, 1.f);
  
}