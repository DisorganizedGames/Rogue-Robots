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
    // uint lightTableMD
    uint pointLightTable;
    uint spotLightTable;
    uint areaLightTable;
    // Static light chunks
    uint staticPointLightOffset;
    uint staticSpotLightOffset;
    uint staticAreaLightOffset;
    // Static light count
    uint staticPointLightCount;
    uint staticSpotLightCount;
    uint staticAreaLightCount;

};

struct ShaderInterop_PerFrameData
{
    matrix viewMatrix;
    matrix projMatrix;
    matrix invProjMatrix;
    float4 camPos;
    float time;

    uint dynPointLightOffset;
    uint dynPointLightCount;

    uint infreqPointLightOffset;
    uint infreqPointLightCount;

    uint dynSpotLightOffset;
    uint dynSpotLightCount;

    uint infreqSpotLightOffset;
    uint infreqSpotLightCount;

    uint dynAreaLightOffset;
    uint dynAreaLightCount;

    uint infreqAreaLightOffset;
    uint infreqAreaLightCount;
};



