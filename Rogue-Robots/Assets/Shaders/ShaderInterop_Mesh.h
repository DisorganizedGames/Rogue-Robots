#pragma once
#include "ShaderInterop_Base.h"

struct ShaderInterop_SubmeshMetadata
{
    uint vertStart;
    uint vertCount;
    uint indexStart;
    uint indexCount;
    uint blendStart;
    uint blendCount;
};
