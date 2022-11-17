#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

#define GROUP_SIZE 64

struct PushConstantElement
{
	uint globalData;
	uint perFrameOffset;
	
	uint globalEmitterTableHandle;
	uint localEmitterTableOffset;
	
	uint particleBufferHandle;
	uint aliveBufferHandle;
	uint toSpawnHandle;
};
CONSTANTS(g_constants, PushConstantElement)

void SpawnParticle(in uint emitterHandle, inout Particle p, in float totTime);

groupshared Emitter g_emitter;
groupshared uint g_spawned;

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	RWStructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	RWStructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.globalEmitterTableHandle];
	RWStructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.aliveBufferHandle];
	RWStructuredBuffer<float> toSpawnBuffer = ResourceDescriptorHeap[g_constants.toSpawnHandle];
	
	StructuredBuffer<ShaderInterop_GlobalData> globalDataTable = ResourceDescriptorHeap[g_constants.globalData];
	ShaderInterop_GlobalData globalData = globalDataTable[0];

	StructuredBuffer<ShaderInterop_PerFrameData> perFrameTable = ResourceDescriptorHeap[globalData.perFrameTable];
	ShaderInterop_PerFrameData perFrame = perFrameTable[g_constants.perFrameOffset];

	uint emitterIdx = g_constants.localEmitterTableOffset + groupID.x;
	if (threadID.x == 0)
	{
		g_emitter = emitterBuffer[emitterIdx];
	}
	
	if (g_emitter.alive == 0)
		return;
	
	if (threadID.x == 0)
	{
		toSpawnBuffer[groupID.x] += perFrame.deltaTime * g_emitter.rate;
		g_spawned = 0;
	}
	
	AllMemoryBarrierWithGroupSync();
	
	if (toSpawnBuffer[groupID.x] >= 1.f)
	{
		for (uint i = threadID.x; i < toSpawnBuffer[groupID.x]; i += GROUP_SIZE)
		{
			uint lastAlive;
			InterlockedAdd(aliveCounter[0], 1, lastAlive);
		
			if (lastAlive < MAX_PARTICLES_ALIVE)
			{
				SpawnParticle(groupID.x, particleBuffer[lastAlive], perFrame.time);
				InterlockedAdd(g_spawned, 1);

			}
		}
	}

	AllMemoryBarrierWithGroupSync();

	if (threadID.x == 0)
	{
		toSpawnBuffer[0] -= g_spawned;
		if (aliveCounter[0] > MAX_PARTICLES_ALIVE)
		{
			aliveCounter[0] = MAX_PARTICLES_ALIVE;
		}
	}
	
	if (globalID == 0)
	{
		aliveCounter[1] = aliveCounter[0];
		aliveCounter[0] = 0;
	}
}

void SpawnParticle(in uint emitterHandle, inout Particle p, in float totTime)
{
	p.emitterHandle = emitterHandle;
	p.pos = g_emitter.pos;
	float xVel = cos(totTime * 30) * 5.f / 4.f;
	float zVel = sin(totTime * 30) * 5.f / 4.f;
	p.vel = float3(xVel, 5, zVel);
	p.size = 0.1;
	p.age = 0;
}

