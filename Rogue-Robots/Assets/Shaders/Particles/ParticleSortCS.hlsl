#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

#define GROUP_SIZE 256

#define LOCAL_PASS 0
#define GLOBAL_PASS 1
#define LOCAL_DISPERSE 2

#define SWAP(type, array, idx1, idx2) {\
	type tmp = array[idx1];				\
	array[idx1] = array[idx2];			\
	array[idx2] = tmp; }

struct PushConstantElement
{
	uint groupSize;
	uint mode;
	uint swapDistance; // or local disperse start (256)
	uint particleBufferHandle;
	uint aliveBufferHandle;
	
	uint globalDataHandle;
	uint perFrameOffset;
};
CONSTANTS(g_constants, PushConstantElement)

groupshared Particle g_localParticles[GROUP_SIZE << 1];
groupshared float g_distances[GROUP_SIZE << 1]; // Squared distances
groupshared float3 g_cameraPos;

void LocalPass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize, in uint globalID);
void GlobalSwapPass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize);
void LocalDispersePass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize);

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint globalID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	RWStructuredBuffer<Particle> particleBuffer = ResourceDescriptorHeap[g_constants.particleBufferHandle];
	RWStructuredBuffer<uint> aliveCounter = ResourceDescriptorHeap[g_constants.aliveBufferHandle];
	
	StructuredBuffer<ShaderInterop_GlobalData> globalDataTable = ResourceDescriptorHeap[g_constants.globalDataHandle];
	ShaderInterop_GlobalData globalData = globalDataTable[0];

	StructuredBuffer<ShaderInterop_PerFrameData> perFrameTable = ResourceDescriptorHeap[globalData.perFrameTable];
	ShaderInterop_PerFrameData perFrame = perFrameTable[g_constants.perFrameOffset];
	
	if (threadID.x == 0)
	{
		//float4x4 camMat = perFrame.invViewMatrix;
		//g_cameraPos = camMat.camPo;
		g_cameraPos = perFrame.camPos;
	}
	
	uint mergeGroupSize = g_constants.groupSize;

	GroupMemoryBarrierWithGroupSync();
	
	if (g_constants.mode == LOCAL_PASS)
	{
		LocalPass(particleBuffer, mergeGroupSize, globalID);
	}
}

void LocalPass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize, in uint globalID)
{
	uint index = globalID * 2;
	
	if (index >= MAX_PARTICLES_ALIVE)
		return;
	
	// Copy particle data to groupshared array
	g_localParticles[index] = particleBuffer[index];
	g_localParticles[index + 1] = particleBuffer[index + 1];
		
	// Write particle -> camera distances to groupshared array
	float3 distVec = g_localParticles[index].pos - g_cameraPos;
	g_distances[index] = g_localParticles[index].alive ? dot(distVec, distVec) : -1.f;
	
	distVec = g_localParticles[index+1].pos - g_cameraPos;
	g_distances[index + 1] = g_localParticles[index + 1].alive ? dot(distVec, distVec) : -1.f;
	
	
	// Perform local pass
	for (uint group = 2; group <= groupSize; group <<= 1)
	{
		uint swapDist = group >> 1; // The swap distance is half that of the group size
		
		uint groupIdx = globalID / swapDist;
		
		index = 2 * groupIdx * swapDist + (globalID % swapDist);
		
		int dir = 1 - (2 * ((index / group) % 2)); // For the bitonic-ness of the sequence (1 if normal, -1 if reversed)
		
		uint swapIndex = index + swapDist;
		
		GroupMemoryBarrierWithGroupSync();
		
		// Perform compare-and-swap (we want greater distances first) (dir reverses for bitonic-ness)
		bool shouldSwap = dir * g_distances[index] < dir * g_distances[swapIndex];
		
		if (shouldSwap)
		{
			SWAP(Particle, g_localParticles, index, swapIndex);
			SWAP(float, g_distances, index, swapIndex);
		}
		
		
		// Disperse pass
		for (uint disperseGroup = group >> 1; disperseGroup >= 2; disperseGroup >>= 1)
		{
			swapDist = disperseGroup >> 1;
			groupIdx = globalID / swapDist;
			index = 2 * groupIdx * swapDist + (globalID % swapDist);
			swapIndex = index + swapDist;

			GroupMemoryBarrierWithGroupSync();
			
			shouldSwap = dir * g_distances[index] < dir * g_distances[swapIndex];
			
			if (shouldSwap)
			{
				SWAP(Particle, g_localParticles, index, swapIndex);
				SWAP(float, g_distances, index, swapIndex);
			}
		}
	}
	
	GroupMemoryBarrierWithGroupSync();
	
	// Copy groupshared memory back to main array
	particleBuffer[index] = g_localParticles[index];
	particleBuffer[index + 1] = g_localParticles[index + 1];
}

