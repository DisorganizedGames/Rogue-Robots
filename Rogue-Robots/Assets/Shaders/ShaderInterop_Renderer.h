#pragma once
#include "ShaderInterop_Base.h"

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
    matrix projMatrix;
    matrix invProjMatrix;
    float4 camPos;
    float time;

    // Offset into light tables to chunk start
    ShaderInterop_LightOffsets pointLightOffsets;
    ShaderInterop_LightOffsets spotLightOffsets;
    ShaderInterop_LightOffsets areaLightOffsets;
};



