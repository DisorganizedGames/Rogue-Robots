#pragma once
#include "../../Handles/HandleAllocator.h"
#include "../RenderDevice.h"
#include "CommonIncludes_DX12.h"

#include "Utilities/DX12DescriptorChunk.h"
#include "Utilities/DX12Fence.h"

namespace D3D12MA { class Allocator; class Allocation; class Pool; }
class DX12DescriptorManager;
class DX12Queue;

namespace DOG::gfx
{
	/*
		Interfaces are used to fully decouple .cpp AND .h file.
		Impl will depend on the header too. Essentially pImpl.
	*/
	class Swapchain_DX12;
	class RenderDevice_DX12 final : public RenderDevice
	{
	public:
		RenderDevice_DX12(ComPtr<ID3D12Device5> device, IDXGIAdapter* adapter, bool debug, UINT numBackBuffers);
		~RenderDevice_DX12();

		Monitor GetMonitor() override;
		const GPUPoolMemoryInfo& GetPoolMemoryInfo(MemoryPool pool);
		const GPUTotalMemoryInfo& GetTotalMemoryInfo();

		u64 GetTotalTextureSize(const TextureDesc& desc);

		Swapchain* CreateSwapchain(void* hwnd, u8 numBuffers);
		Buffer CreateBuffer(const BufferDesc& desc, MemoryPool = {});
		Texture CreateTexture(const TextureDesc& desc, MemoryPool = {});
		Pipeline CreateGraphicsPipeline(const GraphicsPipelineDesc& desc);
		Pipeline CreateComputePipeline(const ComputePipelineDesc& desc);
		RenderPass CreateRenderPass(const RenderPassDesc& desc);
		BufferView CreateView(Buffer buffer, const BufferViewDesc& desc);
		TextureView CreateView(Texture texture, const TextureViewDesc& desc);
		MemoryPool CreateMemoryPool(const MemoryPoolDesc& desc);

		// Aliased
		std::vector<Texture> CreateAliasedTextures(const std::vector<TextureDesc>& descs, MemoryPool pool = {});

		// Free/recycle when appropriate! Sensitive resources that may be in-flight
		void FreeBuffer(Buffer handle);
		void FreeTexture(Texture handle);
		void FreePipeline(Pipeline handle);
		void FreeRenderPass(RenderPass handle);
		void FreeView(BufferView handle);
		void FreeView(TextureView handle);
		void FreeMemoryPool(MemoryPool handle);
		void RecycleSync(SyncReceipt receipt);
		void RecycleCommandList(CommandList handle);

		CommandList AllocateCommandList(QueueType queue = QueueType::Graphics);

		u32 GetGlobalDescriptor(BufferView view) const;
		u32 GetGlobalDescriptor(TextureView view) const;

		

		void Flush();

		// Consumes the receipt (automatic recycling)
		void WaitForGPU(SyncReceipt receipt);

		u8* Map(Buffer handle, u32 subresource = 0, std::pair<u32, u32> readRange = { 0, 0 });
		void Unmap(Buffer handle, u32 subresource = 0, std::pair<u32, u32> writtenRange = { 0, 0 });

		/*
			Sensitive commands start
			==================================
		*/
		void Cmd_SetIndexBuffer(CommandList list,
			Buffer ib);

		void Cmd_Draw(CommandList list,
			u32 vertsPerInstance,
			u32 instanceCount,
			u32 vertStart,
			u32 instanceStart);

		void Cmd_DrawIndexed(CommandList list,
			u32 indicesPerInstance,
			u32 instanceCount,
			u32 indexStart,
			u32 vertStart,
			u32 instanceStart);

		void Cmd_Dispatch(CommandList list,
			u32 threadGroupCountX,
			u32 threadGroupCountY,
			u32 threadGroupCountZ);

		void Cmd_SetPipeline(CommandList list,
			Pipeline pipeline);

		void Cmd_Barrier(CommandList list,
			std::span<GPUBarrier> barriers);

		void Cmd_BeginRenderPass(CommandList list,
			RenderPass rp);

		void Cmd_EndRenderPass(CommandList list);

		void Cmd_UpdateShaderArgs(CommandList list,
			QueueType targetQueue,
			const ShaderArgs& args);

		void Cmd_CopyBuffer(CommandList list,
			Buffer dst,
			u32 dstOffset,
			Buffer src,
			u32 srcOffset,
			u32 size);

		void Cmd_CopyBufferToImage(CommandList list,
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
			u32 srcRowPitch);

		void Cmd_SetViewports(CommandList list,
			Viewports vps);

		void Cmd_SetScissorRects(CommandList list,
			ScissorRects rects);

		void Cmd_ClearUnorderedAccessFLOAT(CommandList list,
			BufferView view, std::array<f32, 4> clear, const ScissorRects& rects);

		void Cmd_ClearUnorderedAccessFLOAT(CommandList list,
			TextureView view, std::array<f32, 4> clear, const ScissorRects& rects);

		void Cmd_ClearUnorderedAccessUINT(CommandList list,
			BufferView view, std::array<u32, 4> clear, const ScissorRects& rects);

		void Cmd_ClearUnorderedAccessUINT(CommandList list,
			TextureView view, std::array<u32, 4> clear, const ScissorRects& rects);


		/*
			Sensitive commands end
			===================================================
		*/


		// Multi-submit
		std::optional<SyncReceipt> SubmitCommandLists(
			std::span<CommandList> lists,
			QueueType queue = QueueType::Graphics,
			std::optional<SyncReceipt> incoming_sync = std::nullopt,		// Synchronize with prior to command list execution
			bool generate_sync = false);									// Generate sync after command list execution

		// Single submit helper
		std::optional<SyncReceipt> SubmitCommandList(
			CommandList list,
			QueueType queue = QueueType::Graphics,
			std::optional<SyncReceipt> incoming_sync = std::nullopt,		// Synchronize with prior to command list execution
			bool generate_sync = false);									// Generate sync after command list execution








		// Implementation interface
	public:
		ID3D12CommandQueue* GetQueue(D3D12_COMMAND_LIST_TYPE type);

		Texture RegisterSwapchainTexture(ComPtr<ID3D12Resource> texture);
		void SetClearColor(Texture tex, const std::array<float, 4>& clearColor);

		// For ImGUI
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetReservedResourceHandle() const;
		ID3D12Device5* GetDevice() const { return m_device.Get(); }
		ID3D12DescriptorHeap* GetMainResourceDH() const;
		ID3D12GraphicsCommandList4* GetListForExternal(CommandList cmdl);

		ID3D12CommandQueue* GetQueue();
		D3D12_CPU_DESCRIPTOR_HANDLE GetReservedRTV(u8 offset);

		// Helpers
	private:
		struct CommandAtorAndList
		{
			ComPtr<ID3D12CommandAllocator> ator;
			ComPtr<ID3D12GraphicsCommandList4> list;
			QueueType queueType{ QueueType::Graphics };

			void Reset()
			{
				ator->Reset();
				list->Reset(ator.Get(), nullptr);
			}

			void Close()
			{
				list->Close();
			}
		};

		struct SyncPrimitive
		{
			DX12Fence fence;
		};



		// Internal storage
	private:
		struct GPUResource_Storage
		{
			ComPtr<D3D12MA::Allocation> alloc;
			ComPtr<ID3D12Resource> resource;

			virtual ~GPUResource_Storage() = default;
		};

		struct Buffer_Storage : public GPUResource_Storage
		{
			BufferDesc desc;
			u8* mappedData{ nullptr };
		};

		struct Texture_Storage : public GPUResource_Storage
		{
			TextureDesc desc;
		};

		struct MemoryPool_Storage
		{
			MemoryPoolDesc desc;
			ComPtr<D3D12MA::Pool> pool;
			GPUPoolMemoryInfo info;
		};

		struct BufferView_Storage
		{
			Buffer buf;
			ViewType type{ ViewType::None };
			DX12DescriptorChunk view;
			std::optional<DX12DescriptorChunk> uavClear;

			BufferView_Storage(Buffer buf_in, ViewType type_in, const DX12DescriptorChunk& descriptor) : buf(buf_in), type(type_in), view(descriptor) {}
		};

		struct TextureView_Storage
		{
			Texture tex;
			ViewType type{ ViewType::None };
			DX12DescriptorChunk view;
			std::optional<DX12DescriptorChunk> uavClear;


			TextureView_Storage(Texture tex_in, ViewType type_in, const DX12DescriptorChunk& descriptor) : tex(tex_in), type(type_in), view(descriptor) {}
		};

		struct Pipeline_Storage
		{
			GraphicsPipelineDesc desc;
			D3D_PRIMITIVE_TOPOLOGY topology{};
			ComPtr<ID3D12PipelineState> pipeline;

			// We could use variant here for pipelines but since compute storage is small we'll just keep it this way
			bool isCompute{ false };
			ComputePipelineDesc computeDesc;
		};

		struct RenderPass_Storage
		{
			RenderPassDesc desc;
			std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> renderTargets;
			std::optional<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC> depthStencil{};
			D3D12_RENDER_PASS_FLAGS flags{};

		};

		struct CommandList_Storage
		{
			CommandAtorAndList pair;
			bool closed{ false };
		};

	private:
		void CreateQueues();
		void InitDMA(IDXGIAdapter* adapter);
		void InitRootsig();
		std::vector<D3D12_STATIC_SAMPLER_DESC> GrabStaticSamplers();
		DX12Queue* GetQueue(QueueType type);
		D3D12_COMMAND_LIST_TYPE GetListType(QueueType queue);


	private:
		ComPtr<ID3D12Device5> m_device;
		bool m_debugOn{ false };

		std::unique_ptr<DX12Queue> m_directQueue, m_copyQueue;
		ComPtr<D3D12MA::Allocator> m_dma;

#ifdef GPU_VALIDATION_ON
		std::unordered_map<u64, std::unordered_map<u32, std::vector<ComPtr<ID3D12Resource>>>> m_mapping;

#endif
		ComPtr<ID3D12RootSignature> m_gfxRsig;
		std::unique_ptr<DX12DescriptorManager> m_descriptorMgr;
		

		HandleAllocator m_rhp;

		std::vector<std::optional<MemoryPool_Storage>> m_memoryPools;
		std::vector<std::optional<Buffer_Storage>> m_buffers;
		std::vector<std::optional<Texture_Storage>> m_textures;
		std::vector<std::optional<BufferView_Storage>> m_bufferViews;
		std::vector<std::optional<TextureView_Storage>> m_textureViews;
		std::vector<std::optional<Pipeline_Storage>> m_pipelines;
		std::vector<std::optional<RenderPass_Storage>> m_renderPasses;
		std::vector<std::optional<CommandList_Storage>> m_cmdls;
		std::vector<std::optional<SyncPrimitive>> m_syncs;

		std::unordered_map<QueueType, std::queue<CommandAtorAndList>> m_recycledAtorAndLists;
		std::queue<SyncPrimitive> m_recycledSyncs;

		// Important that this is destructed before resources and descriptor managers (need to free underlying texture)
		std::unique_ptr<Swapchain_DX12> m_swapchain;

		// ImGUI
		std::optional<DX12DescriptorChunk> m_reservedDescriptor;

		//Ui
		std::optional<DX12DescriptorChunk> m_d2dReservedDescriptor;
		GPUTotalMemoryInfo m_totalMemoryInfo;
	};
}

