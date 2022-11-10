#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

#define GROUP_SIZE 128

struct PushConstantElement
{
	uint globalData;
	uint perFrameOffset;

	uint emitterBufferHandle;
	uint particleBufferHandle;
	uint aliveBufferHandle;
};
CONSTANTS(g_constants, PushConstantElement)

void SpawnParticle(in uint emitterHandle, inout Particle p);

groupshared Emitter g_emitter;

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID)
{
	RWStructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	RWStructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.emitterBufferHandle];
	RWStructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.aliveBufferHandle];

	StructuredBuffer<ShaderInterop_GlobalData> globalDataTable = ResourceDescriptorHeap[g_constants.globalData];
	ShaderInterop_GlobalData globalData = globalDataTable[0];

	StructuredBuffer<ShaderInterop_PerFrameData> perFrameTable = ResourceDescriptorHeap[globalData.perFrameTable];
	ShaderInterop_PerFrameData perFrame = perFrameTable[g_constants.perFrameOffset];
	
	uint alive = aliveCounter[0];
	for (int i = threadID.x; i < alive; i += GROUP_SIZE)
	{
		Particle p = particleBuffer[i];
		Emitter e = emitterBuffer[p.emitterHandle];
		
		p.age += perFrame.deltaTime;
		
		if (p.age < e.lifetime)
		{
			p.pos += p.vel * perFrame.deltaTime;
			p.vel -= float3(0, 9.82 * perFrame.deltaTime, 0);
		}
		particleBuffer[i] = p;
	}
}

