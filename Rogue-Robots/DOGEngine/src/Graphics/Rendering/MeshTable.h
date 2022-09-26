#pragma once
#include "../Handles/HandleAllocator.h"
#include "../../Core/Types/GraphicsTypes.h"
#include "GPUTable.h"

namespace DOG::gfx
{
	class RenderDevice;
	class GPUGarbageBin;
	class UploadContext;

	class MeshTable
	{
	public:
		struct MemorySpecification
		{
			u32 maxTotalSubmeshes{ 0 };
			u32 maxNumIndices{ 0 };
			std::unordered_map<VertexAttribute, u32> maxSizePerAttribute;
		};

		struct MeshSpecification
		{
			std::unordered_map<VertexAttribute, std::span<u8>> vertexDataPerAttribute;
			std::span<u32> indices;
			std::vector<SubmeshMetadata> submeshData;
		};

	public:

		MeshTable(RenderDevice* rd, GPUGarbageBin* bin, const MemorySpecification& spec, bool async = false);

		// Mesh loaded to staging --> Will be uploaded to GPU by upload context! :)
		MeshContainer LoadMesh(const MeshSpecification& spec, UploadContext& ctx);

		void FreeMesh(Mesh handle);

		// Get descriptor to attribute table
		u32 GetAttributeDescriptor(VertexAttribute attr) const;

		// Get descriptor to submesh metadata table
		u32 GetSubmeshDescriptor() const;

		// Grab metadata for specific submesh on CPU
		const SubmeshMetadata& GetSubmeshMD_CPU(Mesh mesh, u32 submesh);

		// Grab local offset for the submesh in the submesh table
		u32 GetSubmeshMD_GPU(Mesh mesh, u32 submesh);

		Buffer GetIndexBuffer();

	private:
		/*
			Attributes handled separately in implemention.
			Have to conform to GPUTable.
			This is okay, since vertex attributes won't change often.

			The interface still works with the enums.
		*/
		static constexpr u32 POS_STRIDE = sizeof(f32) * 3;
		static constexpr u32 UV_STRIDE = sizeof(f32) * 2;
		static constexpr u32 NOR_STRIDE = sizeof(f32) * 3;
		static constexpr u32 TAN_STRIDE = sizeof(f32) * 3;
		static constexpr u32 BLEND_STRIDE = (sizeof(i32)+sizeof(f32))*4;

		struct SubmeshHandle { u64 handle{ 0 }; friend class TypedHandlePool; };

		struct IndexHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct PositionHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct UVHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct NormalHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct TangentHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct BlendHandle { u64 handle{ 0 }; friend class TypedHandlePool; };

		struct Mesh_Storage
		{
			PositionHandle pos;
			UVHandle uv;
			NormalHandle nor;
			TangentHandle tan;
			BlendHandle blend;
			IndexHandle idx;


			std::vector<SubmeshMetadata> mdsCpu;

			SubmeshHandle mdsGpu;
		};

	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };

		HandleAllocator m_handleAtor;
		std::vector<std::optional<Mesh_Storage>> m_resources;

		// Submesh metadata for vertex pulling
		std::unique_ptr<GPUTableDeviceLocal<SubmeshHandle>> m_submeshTable;

		// Vertex attributes
		std::unique_ptr<GPUTableDeviceLocal<IndexHandle>> m_indexTable;
		std::unique_ptr<GPUTableDeviceLocal<PositionHandle>> m_positionTable;
		std::unique_ptr<GPUTableDeviceLocal<UVHandle>> m_uvTable;
		std::unique_ptr<GPUTableDeviceLocal<NormalHandle>> m_normalTable;
		std::unique_ptr<GPUTableDeviceLocal<TangentHandle>> m_tangentTable;
		std::unique_ptr<GPUTableDeviceLocal<BlendHandle>> m_blendTable;




	};
}