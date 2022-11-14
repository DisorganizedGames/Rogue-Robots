#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

struct PushConstantElement
{
	uint globalData;
	uint perFrameOffset;

	uint globalEmitterTableHandle;
	uint localEmitterTableOffset;

	uint particleBufferHandle;
	uint particlesAliveHandle;
};
CONSTANTS(g_constants, PushConstantElement);

struct PerDrawData
{
	uint a;
};

ConstantBuffer<PerDrawData> perDrawData : register(b1, space0);

PS_IN main(uint vID: SV_VertexID, uint iID: SV_InstanceID)
{
	PS_IN output;

	StructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	StructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.particlesAliveHandle];

	StructuredBuffer<ShaderInterop_GlobalData> globalDataTable = ResourceDescriptorHeap[g_constants.globalData];
	ShaderInterop_GlobalData globalData = globalDataTable[0];

	StructuredBuffer<ShaderInterop_PerFrameData> perFrameTable = ResourceDescriptorHeap[globalData.perFrameTable];
	ShaderInterop_PerFrameData perFrame = perFrameTable[g_constants.perFrameOffset];
	
	output.particleID = iID;

	if (iID >= aliveCounter[0])
	{
		output.pos = float4(0, 0, 0, 1);
		output.tex = float2(0, 0);
		return output;
	}

	matrix viewMatrix = perFrame.viewMatrix;
	float3 camRight = viewMatrix[0].xyz;
	float3 camUp = viewMatrix[1].xyz;

	Particle p = particleBuffer[iID];
	output.pos = float4(p.pos, 1);

	float4 rightOffset = float4(camRight, 0) * p.size;
	float4 upOffset = float4(camUp, 0) * p.size;

	switch (vID)
	{
	case 0:
	case 3:
		output.pos += -rightOffset + upOffset;
		output.tex = float2(0, 0);
		break;
	case 1:
		output.pos += rightOffset + upOffset;
		output.tex = float2(1, 0);
		break;
	case 2:
	case 4:
		output.pos += rightOffset - upOffset;
		output.tex = float2(1, 1);
		break;
	case 5:
		output.pos += -rightOffset - upOffset;
		output.tex = float2(0, 1);
		break;
	}
	
	output.pos.w = 1;
	output.pos = mul(perFrame.projMatrix, mul(perFrame.viewMatrix, output.pos));
	
	//output.pos = float4(rightOffset);
	//output.tex = float2(p.size, p.size);

	return output;
}

