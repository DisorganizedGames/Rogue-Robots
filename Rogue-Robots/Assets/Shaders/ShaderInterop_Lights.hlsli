#pragma once
#include "ShaderInterop_Base.h"

struct ShaderInterop_Range
{
    uint offset;
    uint count;
};

#define LOCAL_LIGHT_MAX_SIZE 255
struct ShaderInterop_LocalLightBuffer
{
    uint count;
    uint lightIndices[LOCAL_LIGHT_MAX_SIZE];
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
    float3 position;
    float radius;
    float3 color;
    float strength;
};

struct ShaderInterop_SpotLight
{
    float4 position;
    float3 color;
    float cutoffAngle;
    float3 direction;
    float strength;
    uint id;
};

struct ShaderInterop_AreaLight
{
    
};

struct SpotlightData
{
    matrix viewMatrix;
    matrix projectionMatrix;
    float4 worldPosition;
    float3 color;
    float cutoffAngle;
    float3 direction;
    float strength;
    bool isShadowCaster;
    uint isPlayer; 
    float2 padding;
};