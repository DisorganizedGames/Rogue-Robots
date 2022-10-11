#pragma once
#include "ShaderInterop_Base.h"

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