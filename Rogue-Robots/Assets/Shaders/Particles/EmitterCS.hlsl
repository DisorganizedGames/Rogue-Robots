#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"
#include "../ShaderInterop_Samplers.hlsli"

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
	uint noiseTextureHandle;
};
CONSTANTS(g_constants, PushConstantElement)

void SpawnParticle(in uint emitterHandle, inout Particle p, in float totTime, in Texture1D noiseTex);

groupshared Emitter g_emitter;
groupshared uint g_spawned;

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	RWStructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	RWStructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.globalEmitterTableHandle];
	RWStructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.aliveBufferHandle];
	RWStructuredBuffer<float> toSpawnBuffer = ResourceDescriptorHeap[g_constants.toSpawnHandle];
	Texture1D noiseTexture = ResourceDescriptorHeap[g_constants.noiseTextureHandle];
	
	StructuredBuffer<ShaderInterop_GlobalData> globalDataTable = ResourceDescriptorHeap[g_constants.globalData];
	ShaderInterop_GlobalData globalData = globalDataTable[0];

	StructuredBuffer<ShaderInterop_PerFrameData> perFrameTable = ResourceDescriptorHeap[globalData.perFrameTable];
	ShaderInterop_PerFrameData perFrame = perFrameTable[g_constants.perFrameOffset];

	uint emitterIdx = g_constants.localEmitterTableOffset + groupID.x;
	if (threadID.x == 0)
	{
		g_emitter = emitterBuffer[emitterIdx];
	}
	
	GroupMemoryBarrierWithGroupSync();
	
	if (g_emitter.alive == 0)
	{
		if (threadID.x == 0)
			toSpawnBuffer[groupID.x] = 0.f;
		
		return;
	}
	
	if (threadID.x == 0)
	{
		toSpawnBuffer[groupID.x] += perFrame.deltaTime * g_emitter.rate;
		g_spawned = 0;
	}
	
	AllMemoryBarrierWithGroupSync();
	
	if (toSpawnBuffer[groupID.x] >= 1.f)
	{
		for (uint i = threadID.x; i < toSpawnBuffer[groupID.x] - 1.f; i += GROUP_SIZE)
		{
			uint lastAlive;
			InterlockedAdd(aliveCounter[0], 1, lastAlive);
		
			if (lastAlive < MAX_PARTICLES_ALIVE)
			{
				float lerpedTime = perFrame.time - perFrame.deltaTime;
				float lerper = (toSpawnBuffer[groupID.x] - i) / (float)toSpawnBuffer[groupID.x];
				lerpedTime = lerp(lerpedTime, perFrame.time, lerper);
				
				SpawnParticle(groupID.x, particleBuffer[lastAlive], lerpedTime, noiseTexture);
				InterlockedAdd(g_spawned, 1);

			}
		}
	}

	AllMemoryBarrierWithGroupSync();

	if (threadID.x == 0)
	{
		toSpawnBuffer[groupID.x] -= g_spawned;
		if (aliveCounter[0] > MAX_PARTICLES_ALIVE)
		{
			aliveCounter[0] = MAX_PARTICLES_ALIVE;
		}
	}
}

void SpawnCone(inout Particle p, in Texture1D noiseTex, in float seed);
void SpawnCylinder(inout Particle p, in Texture1D noiseTex, in float seed);
void SpawnBox(inout Particle p, in Texture1D noiseTex, in float seed);
void SpawnDefault(inout Particle p, in float totTime);
float2 RandPointOnDisc(in float radius, in Texture1D noiseTex, in float seed);

void SpawnParticle(in uint emitterHandle, inout Particle p, in float totTime, in Texture1D noiseTex)
{
	p.emitterHandle = emitterHandle;
	
	switch (g_emitter.spawnType)
	{
	case PARTICLE_SPAWN_CONE:
		SpawnCone(p, noiseTex, totTime);
		break;
	case PARTICLE_SPAWN_CYLINDER:
		SpawnCylinder(p, noiseTex, totTime);
		break;
	case PARTICLE_SPAWN_AABB:
		SpawnBox(p, noiseTex, totTime);
		break;
	case PARTICLE_SPAWN_DEFAULT:
		SpawnDefault(p, totTime);
		break;
	}
	
	p.size = g_emitter.size;
	p.age = 0;
}

void SpawnCone(inout Particle p, in Texture1D noiseTex, in float seed)
{
	float angle = g_emitter.opt1;
	float speed = g_emitter.opt2;

	angle = clamp(angle, 0.f, PIDIV2 - 0.0001f);
	float floatingRadius = sin(angle) / sin(PIDIV2 - angle); // See law of sines given a height of 1
	
	float2 randPoint = RandPointOnDisc(floatingRadius, noiseTex, seed);
	
	// Use the random point to determine the particle's velocity
	float randAngleX = atan(randPoint.x);
	float randAngleZ = atan(randPoint.y);
	
	p.vel = normalize(float3(sin(randAngleX), cos(randAngleX), sin(randAngleZ)));
	
	p.pos = g_emitter.pos;
	p.vel *= g_emitter.opt2;

	p.vel = mul(float4(p.vel, 0), g_emitter.rotationMatrix).xyz;
}

void SpawnCylinder(inout Particle p, in Texture1D noiseTex, in float seed)
{
	float radius = g_emitter.opt1;
	float height = g_emitter.opt2;
	
	float2 randPoint = RandPointOnDisc(radius, noiseTex, seed);
	float randHeight = height * noiseTex.Sample(g_bilinear_wrap_samp, seed * 5).r;
	
	float3 offset = float3(randPoint.x, randHeight - height /2.f, randPoint.y);
	offset = mul(float4(offset, 0), g_emitter.rotationMatrix).xyz;
	
	p.pos = g_emitter.pos + offset;
	p.vel = 0.f.xxx;
}

void SpawnBox(inout Particle p, in Texture1D noiseTex, in float seed)
{
	float3 size = float3(g_emitter.opt1, g_emitter.opt2, g_emitter.opt3);
	
	float3 rands = float3(
		noiseTex.Sample(g_bilinear_wrap_samp, seed).x,
		noiseTex.Sample(g_bilinear_wrap_samp, seed * 2).x,
		noiseTex.Sample(g_bilinear_wrap_samp, seed * 6).x
	);

	float3 offset = size * rands - (size / 2.f);
	offset = mul(float4(offset, 0), g_emitter.rotationMatrix).xyz;
	
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
float2 RandPointOnDisc(in float radius, in Texture1D noiseTex, in float seed)
{
	float randRadius = radius * sqrt(noiseTex.Sample(g_bilinear_wrap_samp, seed).r);
	float randPolar = noiseTex.Sample(g_bilinear_wrap_samp, seed * 2).r * TWOPI;
	
	float pointX = randRadius * cos(randPolar);
	float pointZ = randRadius * sin(randPolar);
	
	return float2(pointX, pointZ);
}

