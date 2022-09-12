#pragma once
#include "../RenderResourceHandles.h"
#include <d3d12.h>

namespace DOG::gfx
{
	struct GPUBarrier
	{
		D3D12_RESOURCE_BARRIER_TYPE type{ D3D12_RESOURCE_BARRIER_TYPE_TRANSITION };

		// Keep unsafe
		u64 resource{ 0 };
		u64 aliasResourceAfter{ 0 };
		bool isBuffer{ false };

		D3D12_RESOURCE_STATES stateBefore{ D3D12_RESOURCE_STATE_COMMON };
		D3D12_RESOURCE_STATES stateAfter{ D3D12_RESOURCE_STATE_COMMON };
		u32 subresource{ 0 };

		static GPUBarrier Transition(Buffer resource, u32 subresource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
		{
			GPUBarrier barrier{};
			barrier.type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.resource = resource.handle;
			barrier.isBuffer = true;
			barrier.subresource = subresource;
			barrier.stateBefore = stateBefore;
			barrier.stateAfter = stateAfter;

			return barrier;
		}

		static GPUBarrier Transition(Texture resource, u32 subresource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
		{
			GPUBarrier barrier{};
			barrier.type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.resource = resource.handle;
			barrier.isBuffer = false;
			barrier.subresource = subresource;
			barrier.stateBefore = stateBefore;
			barrier.stateAfter = stateAfter;

			return barrier;
		}

		static GPUBarrier Aliasing(Texture before, Texture after)
		{
			GPUBarrier barrier{};
			barrier.type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
			barrier.resource = before.handle;
			barrier.aliasResourceAfter = after.handle;
			barrier.isBuffer = false;

			return barrier;
		}

		static GPUBarrier UAV(Texture resource)
		{
			GPUBarrier barrier{};
			barrier.type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.resource = resource.handle;
			barrier.isBuffer = false;

			return barrier;
		}

		static GPUBarrier UAV(Buffer resource)
		{
			GPUBarrier barrier{};
			barrier.type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.resource = resource.handle;
			barrier.isBuffer = true;

			return barrier;
		}

	private:
		GPUBarrier() = default;

	};
}