#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

struct PushConstantElement
{
	uint globalData;
	uint perFrameOffset;

	uint emitterBufferHandle;
	uint particleBufferHandle;
};
CONSTANTS(g_constants, PushConstantElement);

struct PerDrawData
{
	uint a;
};

ConstantBuffer<PerDrawData> perDrawData : register(b1, space0);

float4 main(PS_IN input) : SV_Target0
{
	// Pretty red
	//float r = 244, g = 68, b = 78;
	return float4(input.tex.x, input.tex.y, 1, 1);
}

