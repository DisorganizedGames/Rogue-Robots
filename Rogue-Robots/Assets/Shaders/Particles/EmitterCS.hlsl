#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

#define GROUP_SIZE 64

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
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	RWStructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	RWStructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.emitterBufferHandle];
	RWStructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.aliveBufferHandle];
	
    StructuredBuffer<ShaderInterop_GlobalData> globalDataTable = ResourceDescriptorHeap[g_constants.globalData];
    ShaderInterop_GlobalData globalData = globalDataTable[0];

    StructuredBuffer<ShaderInterop_PerFrameData> perFrameTable = ResourceDescriptorHeap[globalData.perFrameTable];
    ShaderInterop_PerFrameData perFrame = perFrameTable[g_constants.perFrameOffset];

	uint emitterIdx = groupID.x;
    if (threadID.x == 0)
    {
		g_emitter = emitterBuffer[emitterIdx];
        g_emitter.age += perFrame.deltaTime;
        g_emitter.rate = 256;
        g_emitter.pos = float3(106.0f, 80.0f, 31.0f);
        g_emitter.lifetime = 1.5f;
    }
	
    AllMemoryBarrierWithGroupSync();

	for (uint i = threadID.x; i < perFrame.deltaTime * g_emitter.rate; i += GROUP_SIZE)
	{
		uint lastAlive;
		InterlockedAdd(aliveCounter[0], 1, lastAlive);
		
		if (lastAlive < MAX_PARTICLES_ALIVE)
			SpawnParticle(emitterIdx, particleBuffer[lastAlive]);
    }

	AllMemoryBarrierWithGroupSync();

	if (threadID.x == 0)
	{
		if (aliveCounter[0] > MAX_PARTICLES_ALIVE)
	    {
	    	aliveCounter[0] = MAX_PARTICLES_ALIVE;
	    }
    
		emitterBuffer[emitterIdx] = g_emitter;
	}
	
    if (globalID == 0)
    {
        aliveCounter[1] = aliveCounter[0];
        aliveCounter[0] = 0;
    }
}

void SpawnParticle(in uint emitterHandle, inout Particle p)
{
	p.emitterHandle = emitterHandle;
	p.pos = g_emitter.pos;
    float xVel = cos(g_emitter.age*30) * 5;
	float zVel = sin(g_emitter.age*30) * 5;
	p.vel = float3(xVel, 20, zVel);
	p.size = 5;
	p.age = 0;
}

