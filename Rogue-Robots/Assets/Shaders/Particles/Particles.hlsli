#pragma once

#define MAX_PARTICLES_ALIVE 4096

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
	float3 pos;
	uint rate;
	float lifetime;
	float3 pad;
};

struct PS_IN
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	uint particleID : PARTICLE_ID;
};
