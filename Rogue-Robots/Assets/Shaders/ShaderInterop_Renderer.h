#pragma once
#include "ShaderInterop_Base.h"

#define TILED_GROUP_SIZE 16

#define DEBUG_SETTING_LIT 1
#define DEBUG_SETTING_LIGHT_CULLING 2
#define DEBUG_SETTING_LIGHT_CULLING_VISUALIZATION 4

/*
    For non-changing structures derived on renderer startup
*/
struct ShaderInterop_GlobalData
{
    uint meshTableSubmeshMD;
    uint meshTablePos;
    uint meshTableUV;
    uint meshTableNor;
    uint meshTableTan;
    uint meshTableBlend;

    uint perFrameTable;
    uint materialTable;

    // Lights
    uint lightTableMD;
    uint pointLightTable;
    uint spotLightTable;
    uint areaLightTable;
};

struct ShaderInterop_LightOffsets
{
    uint staticOffset;
    uint infreqOffset;
    uint dynOffset;
};

struct ShaderInterop_PerFrameData
{
    matrix viewMatrix;
    matrix invViewMatrix;
    matrix projMatrix;
    matrix invProjMatrix;
    float4 camPos;
    float time;

    // Offset into light tables to chunk start
    ShaderInterop_LightOffsets pointLightOffsets;
    ShaderInterop_LightOffsets spotLightOffsets;
    ShaderInterop_LightOffsets areaLightOffsets;

    float deltaTime;
    float nearClip;
    float farClip;
    float pad;
};



