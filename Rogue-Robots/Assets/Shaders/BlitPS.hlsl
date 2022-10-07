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
    uint finalTexID;
};
CONSTANTS(g_constants, PushConstantElement)

float4 main(VS_OUT input) : SV_TARGET
{
    Texture2D finalTex = ResourceDescriptorHeap[g_constants.finalTexID];
    float3 hdr = finalTex.Sample(g_aniso_samp, input.uv);
    
    float3 ldr = aces_fitted(hdr);      // tone mapping
    ldr = pow(ldr, (1.f / 2.22f).rrr);  // gamma correction
    
    return float4(ldr, 1.f);
}