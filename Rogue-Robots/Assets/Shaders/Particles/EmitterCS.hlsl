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

void SpawnCone(inout Particle p);
void SpawnCylinder(inout Particle p);
void SpawnBox(inout Particle p);
void SpawnDefault(inout Particle p, in float totTime);
float2 RandPointOnDisc(in float radius, in float seed);

void SpawnParticle(in uint emitterHandle, inout Particle p, in float totTime)
{
	p.emitterHandle = emitterHandle;
	
	switch (g_emitter.spawnType)
	{
	case PARTICLE_SPAWN_CONE:
		SpawnCone(p);
		break;
	case PARTICLE_SPAWN_CYLINDER:
		SpawnCylinder(p);
		break;
	case PARTICLE_SPAWN_AABB:
		SpawnBox(p);
		break;
	case PARTICLE_SPAWN_DEFAULT:
		SpawnDefault(p, totTime);
		break;
	}
	
	p.size = 0.1;
	p.age = 0;
}

void SpawnCone(inout Particle p)
{
	float angle = g_emitter.opt1;
	float speed = g_emitter.opt2;

	angle = clamp(angle, 0.f, PIDIV2 - 0.0001f);
	float floatingRadius = sin(angle) / sin(PIDIV2 - angle); // See law of sines given a height of 1
	
	float2 randPoint = RandPointOnDisc(floatingRadius, 1.f);
	
	// Use the random point to determine the particle's velocity
	float randAngleX = atan(randPoint.x);
	float randAngleZ = atan(randPoint.y);
	
	p.vel = normalize(float3(sin(randAngleX), cos(randAngleX), sin(randAngleZ)));
	
	p.pos = g_emitter.pos;
	p.vel *= g_emitter.opt2;
}

void SpawnCylinder(inout Particle p)
{
	float radius = g_emitter.opt1;
	float height = g_emitter.opt2;
	
	float2 randPoint = RandPointOnDisc(radius, 1.f);
	float randHeight = height /* * Sample random 0-1 */;
	
	float3 offset = (randPoint.x, randHeight - height / 2.f, randPoint.y);
	
	p.pos = g_emitter.pos + offset;
	p.vel = 0.f.xxx;
}

void SpawnBox(inout Particle p)
{
	float3 size = float3(g_emitter.opt1, g_emitter.opt2, g_emitter.opt3);
	
	float3 rands = float3(
		1,
		1, 
		1
	);

	float3 offset = size * rands - (size / 2.f);
	p.pos = g_emitter.pos + offset;
	p.vel = 0.f.xxx;
}

void SpawnDefault(inout Particle p, in float totTime)
{
	float xVel = cos(totTime * 30) * 5.f / 4.f;
	float zVel = sin(totTime * 30) * 5.f / 4.f;
	p.pos = g_emitter.pos;
	p.vel = float3(xVel, 5, zVel);
}


// Uniform sampling of a point on a disc
float2 RandPointOnDisc(in float radius, in float seed)
{
	// float rand = noiseTexture.Sample(seed, some_sampler);
	float randRadius = radius * sqrt(1.f); // TODO: Change this to be our random percentage of area
	float randPolar = PI;
	
	float pointX = randRadius * cos(randPolar);
	float pointZ = randRadius * sin(randPolar);
	
	return float2(pointX, pointZ);
}

