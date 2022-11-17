#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"
#include "../ShaderInterop_Samplers.hlsli"

struct PushConstantElement
{
	uint globalData;
	uint perFrameOffset;

	uint globalEmitterTableHandle;
	uint localEmitterTableOffset;

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
	StructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.globalEmitterTableHandle];
	StructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	
	Particle p = particleBuffer[input.particleID];
	Emitter e = emitterBuffer[p.emitterHandle + g_constants.localEmitterTableOffset];
	float age = clamp(p.age, 0, e.lifetime) / e.lifetime;
	
	if (e.textureHandle != 0)
	{
		// Texture tiling
		uint totalTiles = e.texSegX * e.texSegY;
		float stepX = rcp(float(e.texSegX));
		float stepY = rcp(float(e.texSegY));
		
		uint curTile = age * totalTiles;
		uint tileX = curTile % e.texSegX;
		uint tileY = curTile / e.texSegX;
		
		float actTexX = input.tex.x * stepX + stepX * tileX;
		float actTexY = input.tex.y * stepY + stepY * tileY;
		float2 actTex = float2(actTexX, actTexY);
		
		// Draw texture
		Texture2D particleTexture = ResourceDescriptorHeap[e.textureHandle];
		return particleTexture.Sample(g_bilinear_clamp_samp, actTex);
	}
	else
	{
		// Temporary color over lifetime if no texture
		float4 start = float4(1, 0, 0, 1);
		float4 end = float4(0, 0, 1, 1);
		
		return (age * end) + ((1 - age) * start);
	}
	
}

