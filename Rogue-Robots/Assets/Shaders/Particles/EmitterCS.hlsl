#include "../ShaderInterop_Renderer.h"

#define MAX_PARTICLES_PER_EMITTER 1024

struct PushConstantElement
{
    uint emitterBufferHandle;
    uint particleBufferHandle;
};
CONSTANTS(g_constants, PushConstantElement)

struct Particle
{
	uint emitterHandle; // A particle is alive if its emitter handle is non-zero
	float3 pos;
	float3 vel;
	float size;
	float3 color;
	float age;
};

struct Emitter
{
	float3 pos;
	float lifetime;
	uint particlesAlive;
	uint3 padding;
};

void SpawnParticle(inout Emitter e, inout Particle p);

[numthreads(64, 1, 1)]
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	RWStructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	RWStructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.emitterBufferHandle];

	uint emitterIdx = groupID.x;
	uint startParticle = threadID.x + (MAX_PARTICLES_PER_EMITTER * emitterIdx);

	for (uint i = startParticle; i < MAX_PARTICLES_PER_EMITTER; i += 64)
	{
		SpawnParticle(emitterBuffer[emitterIdx], particleBuffer[i]);
		particleBuffer[i].emitterHandle = emitterIdx;
	}
}

void SpawnParticle(inout Emitter e, inout Particle p)
{
	p.pos = e.pos;
	p.vel = float3(0, 0.1, 0);
	p.size = 0.1;
	p.age = 0;

	float r = 244, g = 68, b = 78;
	p.color = float3(r, g, b) / 255.0;
}
