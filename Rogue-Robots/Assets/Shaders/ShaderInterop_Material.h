#pragma once
#include "ShaderInterop_Base.h"

struct ShaderInterop_MaterialElement
{
    uint albedo;
    uint metallicRoughness;
    uint normal;
    uint emissive;

    float4 albedoFactor;
    float4 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
};