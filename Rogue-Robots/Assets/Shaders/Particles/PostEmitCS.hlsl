#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"
#include "../ShaderInterop_Samplers.hlsli"

#define GROUP_SIZE 1

struct PushConstantElement
{
	uint aliveBufferHandle;
};
CONSTANTS(g_constants, PushConstantElement)

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	RWStructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.aliveBufferHandle];
	
	if (globalID == 0)
	{
		aliveCounter[1] = aliveCounter[0];
		aliveCounter[0] = 0;
	}
}

