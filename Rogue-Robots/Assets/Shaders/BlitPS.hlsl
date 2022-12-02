#include "ShaderInterop_Base.h"
#include "ShaderInterop_Samplers.hlsli"
#include "ShaderInterop_Renderer.h"
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
    float bloomStrength;
};
CONSTANTS(g_constants, PushConstantElement)

float4 main(VS_OUT input) : SV_TARGET
{
    Texture2D outlineTex = ResourceDescriptorHeap[g_constants.outline];
    float3 outline = outlineTex.Sample(g_bilinear_clamp_samp, input.uv).rgb;
    
    
    
    Texture2D finalTex = ResourceDescriptorHeap[g_constants.finalTexID];
    float4 hdr4 = finalTex.Sample(g_bilinear_clamp_samp, input.uv);
    float3 hdr = hdr4.rgb;
    float hdrAlpha = hdr4.a;
    
    
    float4 ssaoColor = 1.f.rrrr;
    float ssaoContrib = 1.f;
    if (!(g_constants.ao == 0xffffffff))
    {
        Texture2D<float4> ssao = ResourceDescriptorHeap[g_constants.ao];
        ssaoColor = ssao.Sample(g_point_clamp_samp, input.uv);
        ssaoContrib = uncharted2_filmic(ssaoColor.rgb).r;
    }
   
    float4 bloom = 0.f.rrrr;
    if (!(g_constants.bloom == 0xffffffff))
    {
        Texture2D<float4> bloomTex = ResourceDescriptorHeap[g_constants.bloom];
        bloom = bloomTex.Sample(g_bilinear_clamp_samp, input.uv);
    }

    // Darken the white halos with Reinhard Jodie
    //float ssaoContrib = ssaoColor.r;
    //float ssaoContrib = aces_fitted(ssaoColor.rgb).r;
    //float ssaoContrib = reinhard_jodie(ssaoColor.rgb).r;
    //ssaoContrib = aces_fitted(ssaoContrib.rrr).r;
    
    //return float4(ssaoContrib.rrr, 1.f);
    
    hdr *= ssaoContrib;
    hdr += g_constants.bloomStrength * bloom.rgb;
    
    // Magic number
    if (hdrAlpha < MAGIC_WEAPON_ALPHA_TAG)
        hdr += outline;
    
    float3 ldr = reinhard_jodie(hdr); // tone mapping
    //float3 ldr = aces_fitted(hdr); // tone mapping
    
    ldr = pow(ldr, (1.f / g_constants.gamma).rrr); // gamma correction
    
    
   
    return float4(ldr, 1.f);
}