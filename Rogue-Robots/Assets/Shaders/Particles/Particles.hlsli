#pragma once

#define MAX_PARTICLES_ALIVE (16*1024)

struct Particle
{
	uint emitterHandle;
	float3 pos;
	float age;
	float3 vel;
	float size;
	float3 pad;
};

struct Emitter
{
    uint alive;
	float3 pos;
	float rate;
	float lifetime;
	float age;
	float pad;
};

struct PS_IN
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	uint particleID : PARTICLE_ID;
};
