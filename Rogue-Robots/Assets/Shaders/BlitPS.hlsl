#include "ShaderInterop_Base.h"
#include "ShaderInterop_Samplers.hlsli"
#include "AcesTonemapping.hlsli"

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};


struct PushConstantElement
{
    uint ao;
    uint finalTexID;
    uint bloom;
};
CONSTANTS(g_constants, PushConstantElement)


float4 main(VS_OUT input) : SV_TARGET
{
    Texture2D<float4> ssao = ResourceDescriptorHeap[g_constants.ao];
    float4 ssaoColor = ssao.Sample(g_bilinear_wrap_samp, input.uv);
    
    Texture2D finalTex = ResourceDescriptorHeap[g_constants.finalTexID];
    float3 hdr = finalTex.Sample(g_aniso_samp, input.uv);
    
    Texture2D<float4> bloomTex = ResourceDescriptorHeap[g_constants.bloom];
    float4 bloom = bloomTex.Sample(g_aniso_samp, input.uv);
    
    // Darken the white halos with Reinhard Jodie
    float ssaoContrib = reinhard_jodie(ssaoColor.rgb).r;
    ssaoContrib = reinhard_jodie(ssaoContrib.rrr).r;
    
    hdr *= ssaoContrib;
    hdr += bloom;
    
    float3 ldr = reinhard_jodie(hdr); // tone mapping
    ldr = pow(ldr, (1.f / 2.22f).rrr);  // gamma correction
    
   
    return float4(ldr, 1.f);
}