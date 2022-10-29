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
    float ssaoContrib = ssaoColor.r;
    //float ssaoContrib = aces_fitted(ssaoColor.rgb).r;
    //ssaoContrib = aces_fitted(ssaoContrib.rrr).r;
    
    //return float4(ssaoContrib.rrr, 1.f);
    
    hdr *= ssaoContrib;
    hdr += bloom;
    
    float3 ldr = reinhard_jodie(hdr); // tone mapping
    //float3 ldr = aces_fitted(hdr); // tone mapping
    ldr = pow(ldr, (1.f / 2.22f).rrr);  // gamma correction
    
   
    return float4(ldr, 1.f);
}