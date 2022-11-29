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
    uint input;
};
CONSTANTS(g_constants, PushConstantElement)

float4 main(VS_OUT input) : SV_TARGET
{
    Texture2D tex = ResourceDescriptorHeap[g_constants.input];
    return tex.Sample(g_bilinear_clamp_samp, input.uv);
}