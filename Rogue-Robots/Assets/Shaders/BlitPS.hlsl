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
    float gamma;
    uint bloom;
    uint outline;
};
CONSTANTS(g_constants, PushConstantElement)


float4 main(VS_OUT input) : SV_TARGET
{
    Texture2D outlineTex = ResourceDescriptorHeap[g_constants.outline];
    float3 outline = outlineTex.Sample(g_aniso_samp, input.uv).rgb;
    
    
    
    Texture2D finalTex = ResourceDescriptorHeap[g_constants.finalTexID];
    float3 hdr = finalTex.Sample(g_aniso_samp, input.uv);
    
    float4 ssaoColor = 1.f.rrrr;
    float ssaoContrib = 1.f;
    if (!(g_constants.ao == 0xffffffff))
    {
        Texture2D<float4> ssao = ResourceDescriptorHeap[g_constants.ao];
        ssaoColor = ssao.Sample(g_point_samp, input.uv);
        ssaoContrib = uncharted2_filmic(ssaoColor.rgb).r;
    }
   
    float4 bloom = 0.f.rrrr;
    if (!(g_constants.bloom == 0xffffffff))
    {
        Texture2D<float4> bloomTex = ResourceDescriptorHeap[g_constants.bloom];
        bloom = bloomTex.Sample(g_aniso_samp, input.uv);
    }

    // Darken the white halos with Reinhard Jodie
    //float ssaoContrib = ssaoColor.r;
    //float ssaoContrib = aces_fitted(ssaoColor.rgb).r;
    //float ssaoContrib = reinhard_jodie(ssaoColor.rgb).r;
    //ssaoContrib = aces_fitted(ssaoContrib.rrr).r;
    
    //return float4(ssaoContrib.rrr, 1.f);
    
    hdr *= ssaoContrib;
    hdr += bloom;
    
    
    
    float3 ldr = reinhard_jodie(hdr); // tone mapping
    //float3 ldr = aces_fitted(hdr); // tone mapping
    ldr += outline;
    
    ldr = pow(ldr, (1.f / g_constants.gamma).rrr); // gamma correction
    
    
   
    return float4(ldr, 1.f);
}