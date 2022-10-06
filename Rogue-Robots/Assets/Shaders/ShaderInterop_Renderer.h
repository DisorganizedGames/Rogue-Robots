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
};

struct ShaderInterop_PerFrameData
{
    matrix viewMatrix;
    matrix projMatrix;
    matrix invProjMatrix;
    float4 camPos;
    float time;
};



