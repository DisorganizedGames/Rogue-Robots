#include "Particles.hlsli"
#include "../ShaderInterop_Renderer.h"

#define GROUP_SIZE 256

#define LOCAL_PASS 0
#define GLOBAL_PASS 1
#define LOCAL_DISPERSE 2

#define SWAP(type, array, idx1, idx2) {	\
	type tmp = array[idx1];				\
	array[idx1] = array[idx2];			\
	array[idx2] = tmp; }

struct PushConstantElement
{
	uint groupSize;
	uint mode;
	uint swapDistance; // or local disperse start (1024)
	uint particleBufferHandle;
	uint aliveBufferHandle;
	
	uint globalDataHandle;
	uint perFrameOffset;
};
CONSTANTS(g_constants, PushConstantElement)

groupshared Particle g_localParticles[GROUP_SIZE << 1];
groupshared float g_distances[GROUP_SIZE << 1]; // Squared distances
groupshared float3 g_cameraPos;

void LocalPass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize, in uint globalID, in uint threadID);
void GlobalSwapPass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize, in uint swapDist, in uint globalID);
void LocalDispersePass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize, in uint globalID, in uint offset);

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
		g_cameraPos = perFrame.camPos;
	}
	
	uint groupSize = g_constants.groupSize;

	GroupMemoryBarrierWithGroupSync();
	
	if (g_constants.mode == LOCAL_PASS)
	{
		LocalPass(particleBuffer, groupSize, globalID, groupID.x * GROUP_SIZE * 2);
	}
	else if (g_constants.mode == GLOBAL_PASS)
	{
		GlobalSwapPass(particleBuffer, groupSize, g_constants.swapDistance, globalID);
	}
	else if (g_constants.mode == LOCAL_DISPERSE)
	{
		LocalDispersePass(particleBuffer, groupSize, globalID, groupID.x * GROUP_SIZE * 2);
	}
}

void LocalPass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize, in uint globalID, in uint offset)
{
	uint index = globalID * 2;
	uint offsetIndex = index - offset;
	
	if (index >= MAX_PARTICLES_ALIVE)
		return;
	
	// Copy particle data to groupshared array
	g_localParticles[offsetIndex] = particleBuffer[index];
	g_localParticles[offsetIndex + 1] = particleBuffer[index + 1];
			
	// Write particle -> camera distances to groupshared array
	float3 distVec = g_localParticles[offsetIndex].pos - g_cameraPos;
	g_distances[offsetIndex] = g_localParticles[offsetIndex].alive ? dot(distVec, distVec) : -1.f;
	
	distVec = g_localParticles[offsetIndex + 1].pos - g_cameraPos;
	g_distances[offsetIndex + 1] = g_localParticles[offsetIndex + 1].alive ? dot(distVec, distVec) : -1.f;

	// Perform local pass
	for (uint group = 2; group <= groupSize; group <<= 1)
	{
		uint swapDist = group >> 1; // The swap distance is half that of the group size
		
		uint groupIdx = globalID / swapDist;
		
		index = 2 * groupIdx * swapDist + (globalID % swapDist);
		
		int dir = 1 - (2 * ((index / group) % 2)); // For the bitonic-ness of the sequence (1 if normal, -1 if reversed)
		
		index -= offset;
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
			index = 2 * groupIdx * swapDist + (globalID % swapDist) - offset;
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
	
	AllMemoryBarrierWithGroupSync();
	
	index = globalID * 2;
	
	// Copy groupshared memory back to main array
	particleBuffer[index] = g_localParticles[index - offset];
	particleBuffer[index + 1] = g_localParticles[index - offset + 1];
}

void GlobalSwapPass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize, in uint swapDist, in uint globalID)
{
	uint groupIdx = globalID / swapDist; // This thread is part of some group of size groupSize
	
	uint index = 2 * groupIdx * swapDist + (globalID % swapDist);
	
	uint swapIndex = index + swapDist;
	
	Particle p1 = particleBuffer[index];
	Particle p2 = particleBuffer[swapIndex];
	
	float3 distVec1 = p1.pos - g_cameraPos;
	float3 distVec2 = p2.pos - g_cameraPos;
	float dist1 = p1.alive ? dot(distVec1, distVec1) : -1.f;
	float dist2 = p2.alive ? dot(distVec2, distVec2) : -1.f;

	int dir = 1 - (2 * ((index / groupSize) % 2)); // For the bitonic-ness of the sequence (1 if normal, -1 if reversed)
	
	if (dir * dist1 < dir * dist2)
	{
		SWAP(Particle, particleBuffer, index, swapIndex);
	}
}

void LocalDispersePass(inout RWStructuredBuffer<Particle> particleBuffer, in uint groupSize, in uint globalID, in uint offset)
{
	uint index = globalID * 2;
	uint offsetIndex = index - offset;
	
	// Copy particle data to groupshared array
	g_localParticles[offsetIndex] = particleBuffer[index];
	g_localParticles[offsetIndex + 1] = particleBuffer[index + 1];
		
	// Write particle -> camera distances to groupshared array
	float3 distVec = g_localParticles[offsetIndex].pos - g_cameraPos;
	g_distances[offsetIndex] = g_localParticles[offsetIndex].alive ? dot(distVec, distVec) : -1.f;
	
	distVec = g_localParticles[offsetIndex + 1].pos - g_cameraPos;
	g_distances[offsetIndex + 1] = g_localParticles[offsetIndex + 1].alive ? dot(distVec, distVec) : -1.f;
	
	int dir = 1 - (2 * ((index / groupSize) % 2)); // For the bitonic-ness of the sequence (1 if normal, -1 if reversed)
	
	for (uint disperseGroup = GROUP_SIZE * 2; disperseGroup >= 2; disperseGroup >>= 1)
	{
		uint swapDist = disperseGroup >> 1;
		uint groupIdx = globalID / swapDist;
		index = 2 * groupIdx * swapDist + (globalID % swapDist) - offset;
		uint swapIndex = index + swapDist;
		
		GroupMemoryBarrierWithGroupSync();

		bool shouldSwap = dir * g_distances[index] < dir * g_distances[swapIndex];
			
		if (shouldSwap)
		{
			SWAP(Particle, g_localParticles, index, swapIndex);
			SWAP(float, g_distances, index, swapIndex);
		}
	}
	
	GroupMemoryBarrierWithGroupSync();
	
	index = globalID * 2;
	
	// Copy groupshared memory back to main array
	particleBuffer[index] = g_localParticles[index - offset];
	particleBuffer[index + 1] = g_localParticles[index - offset + 1];
}

