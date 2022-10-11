#pragma once
#include "ShaderInterop_Base.h"

struct ShaderInterop_Range
{
    uint offset;
    uint count;
};

struct ShaderInterop_LightsMetadata
{
    // Point lights (statics, infreqs, dynamics)
    ShaderInterop_Range staticPointLightRange;
    ShaderInterop_Range infreqPointLightRange;
    ShaderInterop_Range dynPointLightRange;
    
    // Spot lights
    ShaderInterop_Range staticSpotLightRange;
    ShaderInterop_Range infreqSpotLightRange;
    ShaderInterop_Range dynSpotLightRange;

    // Area lights
    ShaderInterop_Range staticAreaLightRange;
    ShaderInterop_Range infreqAreaLightRange;
    ShaderInterop_Range dynAreaLightRange;
};

struct ShaderInterop_PointLight
{
    
};

struct ShaderInterop_SpotLight
{
    float4 position;
    float4 color;
    float3 direction;
    float strength;
};

struct ShaderInterop_AreaLight
{
    
};