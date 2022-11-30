#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

#define GROUP_SIZE 256

struct PushConstantElement
{
	uint globalEmitterTableHandle;
	uint localEmitterTableOffset;
	
	uint particleBufferHandle;
	uint aliveBufferHandle;
};
CONSTANTS(g_constants, PushConstantElement)

groupshared Emitter g_emitter;

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID)
{
	RWStructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	RWStructuredBuffer<Emitter> emitterBuffer = ResourceDescriptorHeap[g_constants.globalEmitterTableHandle];
	RWStructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.aliveBufferHandle];
	
	uint prevAlive = aliveCounter[1];

	Particle p = particleBuffer[globalID];
	Emitter e = emitterBuffer[p.emitterHandle + g_constants.localEmitterTableOffset];
	
	bool particleAlive = p.age < e.lifetime && globalID < prevAlive;
	uint waveParticlesAlive = WaveActiveCountBits(particleAlive);
	uint laneIdx = WavePrefixCountBits(particleAlive);
	uint waveStartIdx;
	if (WaveGetLaneIndex() == 0)
	{
		InterlockedAdd(aliveCounter[0], waveParticlesAlive, waveStartIdx);
	}
	waveStartIdx = WaveReadLaneAt(waveStartIdx, 0);
	
	if (particleAlive)
	{
		particleBuffer[waveStartIdx + laneIdx] = p;
	}
}


