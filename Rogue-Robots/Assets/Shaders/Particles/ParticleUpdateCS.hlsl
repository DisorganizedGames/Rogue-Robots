#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

#define GROUP_SIZE 128

struct PushConstantElement
{
	uint globalData;
	uint perFrameOffset;

	uint globalEmitterTableHandle;
	uint localEmitterTableOffset;

	uint particleBufferHandle;
	uint aliveBufferHandle;
};
CONSTANTS(g_constants, PushConstantElement)

void GravityBehavior(inout Particle p, in Emitter e, in float deltaTime);
void GravityPointBehavior(inout Particle p, in Emitter e, in float deltaTime);
void GravityDirectionBehavior(inout Particle p, in Emitter e, in float deltaTime);
void ConstVelocityBehavior(inout Particle p, in Emitter e, in float deltaTime);

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID)
{
	RWStructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	RWStructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.globalEmitterTableHandle];
	RWStructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.aliveBufferHandle];

	StructuredBuffer<ShaderInterop_GlobalData> globalDataTable = ResourceDescriptorHeap[g_constants.globalData];
	ShaderInterop_GlobalData globalData = globalDataTable[0];

	StructuredBuffer<ShaderInterop_PerFrameData> perFrameTable = ResourceDescriptorHeap[globalData.perFrameTable];
	ShaderInterop_PerFrameData perFrame = perFrameTable[g_constants.perFrameOffset];
	
	uint alive = aliveCounter[0];
	
    if (globalID >= alive)
        return;
	
	Particle p = particleBuffer[globalID];
	Emitter e = emitterBuffer[p.emitterHandle + g_constants.localEmitterTableOffset];
	
	p.age += perFrame.deltaTime;

	switch (e.behavior)
	{
	case BEHAVIOR_GRAVITY:
		GravityBehavior(p, e, perFrame.deltaTime);
		break;
	case BEHAVIOR_NO_GRAVITY:
		break;
	case BEHAVIOR_GRAVITY_POINT:
		GravityPointBehavior(p, e, perFrame.deltaTime);
		break;
	case BEHAVIOR_GRAVITY_DIRECTION:
		GravityDirectionBehavior(p, e, perFrame.deltaTime);
		break;
	case BEHAVIOR_CONST_VELOCITY:
		ConstVelocityBehavior(p, e, perFrame.deltaTime);
		break;
	case BEHAVIOR_DEFAULT: 
		e.bopt1 = 9.82f;
		GravityBehavior(p, e, perFrame.deltaTime);
		break;
	}
	
	p.pos += p.vel * perFrame.deltaTime;
	
	particleBuffer[globalID] = p;
}

void GravityBehavior(inout Particle p, in Emitter e, in float deltaTime)
{
	float gravity = e.bopt1;
	
	p.vel -= float3(0, gravity * deltaTime, 0);
}

void GravityPointBehavior(inout Particle p, in Emitter e, in float deltaTime)
{
	float x = e.bopt1;
	float y = e.bopt2;
	float z = e.bopt3;
	float gravity = e.bopt4;
	
	float3 dir = normalize(float3(x, y, z) - p.pos);
	p.vel += dir * gravity * deltaTime;
}

void GravityDirectionBehavior(inout Particle p, in Emitter e, in float deltaTime)
{
}

void ConstVelocityBehavior(inout Particle p, in Emitter e, in float deltaTime)
{
}

