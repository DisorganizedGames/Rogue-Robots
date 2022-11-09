#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

#define GROUP_SIZE 64

struct PushConstantElement
{
	uint emitterBufferHandle;
	uint particleBufferHandle;
	uint aliveBufferHandle;
};
CONSTANTS(g_constants, PushConstantElement)

void SpawnParticle(in uint emitterHandle, inout Particle p, in int idx);

groupshared Emitter g_emitter;

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	RWStructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	RWStructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.emitterBufferHandle];
	RWStructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.aliveBufferHandle];

	uint emitterIdx = groupID.x;
	g_emitter = emitterBuffer[emitterIdx];

	for (uint i = threadID.x; i < 8; i += GROUP_SIZE)
	{
		uint lastAlive;
		InterlockedAdd(aliveCounter[0], 1, lastAlive);

		uint idx = lastAlive % MAX_PARTICLES_ALIVE;
		SpawnParticle(emitterIdx, particleBuffer[idx], (int)idx);
	}

	AllMemoryBarrierWithGroupSync();

	if (threadID.x == 0)
	{
		//if (aliveCounter[0] > MAX_PARTICLES_ALIVE)
		//{
		//	aliveCounter[0] = MAX_PARTICLES_ALIVE;
		//}
		
		// Might not be needed
		//emitterBuffer[emitterIdx] = g_emitter;
		
	}
}

void SpawnParticle(in uint emitterHandle, inout Particle p, in int idx)
{
	p.emitterHandle = emitterHandle;
	p.pos = g_emitter.pos;
	float xVel = cos((idx%360) * 3.14/180) * 7;
	float zVel = sin((idx%360) * 3.14/180) * 7;
	p.vel = float3(xVel, 20, zVel);
	p.size = 5;
	p.age = 0;
}

