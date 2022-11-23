#pragma once

#define MAX_PARTICLES_ALIVE (16*1024)
static const float PI = 3.14159265758979323846f;
static const float PIDIV2 = PI / 2.f;
static const float TWOPI = 2 * PI;

/*
enum class ParticleSpawnType : u8
{
	Cone = 0,
	Cylinder = 1,
	AABB = 2,
	Default = 255,
};
*/
#define PARTICLE_SPAWN_CONE 0
#define PARTICLE_SPAWN_CYLINDER 1
#define PARTICLE_SPAWN_AABB 2
#define PARTICLE_SPAWN_DEFAULT 255

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
	
	uint spawnType;
	float opt1;
	float opt2;
	float opt3;
	
	uint textureHandle;
	uint texSegX;
	uint texSegY;
	
	float4x4 rotationMatrix;
	
	float4 startColor;
	float4 endColor;
	
	uint3 pad;
};

struct PS_IN
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	uint particleID : PARTICLE_ID;
};
