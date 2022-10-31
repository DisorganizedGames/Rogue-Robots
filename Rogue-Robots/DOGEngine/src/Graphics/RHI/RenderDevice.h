#pragma once
#include <d3d12.h>
#include "Swapchain.h"
#include "RHITypes.h"

namespace DOG::gfx
{
	/*
		Interface used for pImpl, fully decouple impl. (header)
	*/
	class RenderDevice
	{
	public:
		virtual Swapchain* CreateSwapchain(void* hwnd, u8 numBuffers) = 0;

		virtual Buffer CreateBuffer(const BufferDesc& desc, MemoryPool pool = {}) = 0;
		virtual Texture CreateTexture(const TextureDesc& desc, MemoryPool pool = {}) = 0;
		virtual Pipeline CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
		virtual Pipeline CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
		virtual RenderPass CreateRenderPass(const RenderPassDesc& desc) = 0;
		virtual BufferView CreateView(Buffer buffer, const BufferViewDesc& desc) = 0;
		virtual TextureView CreateView(Texture texture, const TextureViewDesc& desc) = 0;
		virtual MemoryPool CreateMemoryPool(const MemoryPoolDesc& desc) = 0;


		virtual Monitor GetMonitor() = 0;
		virtual const GPUPoolMemoryInfo& GetPoolMemoryInfo(MemoryPool pool) = 0;
		virtual const GPUTotalMemoryInfo& GetTotalMemoryInfo() = 0;

		// Free/recycle when appropriate! Sensitive resources that may be in-flight
		virtual void FreeBuffer(Buffer handle) = 0;
		virtual void FreeTexture(Texture handle) = 0;
		virtual void FreePipeline(Pipeline handle) = 0;
		virtual void FreeRenderPass(RenderPass handle) = 0;
		virtual void FreeView(BufferView handle) = 0;
		virtual void FreeView(TextureView handle) = 0;
		virtual void RecycleSync(SyncReceipt receipt) = 0;
		virtual void RecycleCommandList(CommandList handle) = 0;
		virtual void FreeMemoryPool(MemoryPool handle) = 0;


		virtual CommandList AllocateCommandList(QueueType queue = QueueType::Graphics) = 0;

		virtual u32 GetGlobalDescriptor(BufferView view) const = 0;
		virtual u32 GetGlobalDescriptor(TextureView view) const = 0;

		virtual void Flush() = 0;

		// Consumes the receipt (automatic recycling)
		virtual void WaitForGPU(SyncReceipt receipt) = 0;

		virtual u8* Map(Buffer handle, u32 subresource = 0, std::pair<u32, u32> readRange = { 0, 0 }) = 0;
		virtual void Unmap(Buffer handle, u32 subresource = 0, std::pair<u32, u32> writtenRange = { 0, 0 }) = 0;

		/*
			Sensitive commands start
			==================================
		*/
		virtual void Cmd_SetIndexBuffer(CommandList list,
			Buffer ib) = 0;

		virtual void Cmd_Draw(CommandList list,
			u32 vertsPerInstance,
			u32 instanceCount,
			u32 vertStart,
			u32 instanceStart) = 0;

		virtual void Cmd_Dispatch(CommandList list,
			u32 threadGroupCountX,
			u32 threadGroupCountY,
			u32 threadGroupCountZ) = 0;

		virtual void Cmd_DrawIndexed(CommandList list,
			u32 indicesPerInstance,
			u32 instanceCount,
			u32 indexStart,
			u32 vertStart,
			u32 instanceStart) = 0;	

		virtual void Cmd_SetPipeline(CommandList list,
			Pipeline pipeline) = 0;

		virtual void Cmd_Barrier(CommandList list,
			std::span<GPUBarrier> barriers) = 0;

		virtual void Cmd_BeginRenderPass(CommandList list,
			RenderPass rp) = 0;

		virtual void Cmd_EndRenderPass(CommandList list) = 0;

		virtual void Cmd_UpdateShaderArgs(CommandList list,
			QueueType targetQueue,
			const ShaderArgs& args) = 0;

		virtual void Cmd_CopyBuffer(CommandList list,
			Buffer dst,
			u32 dstOffset,
			Buffer src,
			u32 srcOffset,
			u32 size) = 0;

		virtual void Cmd_CopyBufferToImage(CommandList list,
			Texture dst,
			u32 dstSubresource,
			std::tuple<u32, u32, u32> dstTopLeft,

			Buffer src,
			// Describe the data in 'src':
			u32 srcOffset,
			DXGI_FORMAT srcFormat,
			u32 srcWidth,
			u32 srcHeight,
			u32 srcDepth,
			u32 srcRowPitch) = 0;

		virtual void Cmd_SetViewports(CommandList list,
			Viewports vps) = 0;

		virtual void Cmd_SetScissorRects(CommandList list,
			ScissorRects rects) = 0;

		virtual void Cmd_ClearUnorderedAccessFLOAT(CommandList list,
			BufferView view, std::array<f32, 4> clear, const ScissorRects& rects) = 0;

		virtual void Cmd_ClearUnorderedAccessFLOAT(CommandList list,
			TextureView view, std::array<f32, 4> clear, const ScissorRects& rects) = 0;

		virtual void Cmd_ClearUnorderedAccessUINT(CommandList list,
			BufferView view, std::array<u32, 4> clear, const ScissorRects& rects) = 0;

		virtual void Cmd_ClearUnorderedAccessUINT(CommandList list,
			TextureView view, std::array<u32, 4> clear, const ScissorRects& rects) = 0;



		/*
			Sensitive commands end
			===================================================
		*/


		// Multi-submit
		virtual std::optional<SyncReceipt> SubmitCommandLists(
			std::span<CommandList> lists,
			QueueType queue = QueueType::Graphics,
			std::optional<SyncReceipt> incoming_sync = std::nullopt,		// Synchronize with prior to command list execution
			bool generate_sync = false) = 0;									// Generate sync after command list execution

		// Single submit helper
		virtual std::optional<SyncReceipt> SubmitCommandList(
			CommandList list,
			QueueType queue = QueueType::Graphics,
			std::optional<SyncReceipt> incoming_sync = std::nullopt,		// Synchronize with prior to command list execution
			bool generate_sync = false) = 0;									// Generate sync after command list execution


		virtual ~RenderDevice() {}
	};
}