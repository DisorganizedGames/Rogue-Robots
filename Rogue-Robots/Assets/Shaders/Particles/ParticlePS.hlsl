#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

struct PushConstantElement
{
	uint globalData;
	uint perFrameOffset;

	uint emitterBufferHandle;
	uint particleBufferHandle;
};
CONSTANTS(g_constants, PushConstantElement);

struct PerDrawData
{
	uint a;
};

ConstantBuffer<PerDrawData> perDrawData : register(b1, space0);

float4 main(PS_IN input) : SV_Target0
{
	StructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.emitterBufferHandle];
	StructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	
	Particle p = particleBuffer[input.particleID];
	Emitter e = emitterBuffer[p.emitterHandle];
	float age = clamp(p.age, 0, e.lifetime) / e.lifetime;

	float4 start = float4(1, 0, 0, 1);
	float4 end = float4(0, 0, 1, 1);
	
	return (age *end + (1-age)*start);
}

