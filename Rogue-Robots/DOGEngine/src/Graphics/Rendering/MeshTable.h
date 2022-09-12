#pragma once
#include "../Handles/HandleAllocator.h"
#include "Types/MeshTypes.h"
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
			std::unordered_map<VertexAttributeTemp, u32> maxSizePerAttribute;
		};

		struct MeshSpecification
		{
			std::unordered_map<VertexAttributeTemp, std::span<u8>> vertexDataPerAttribute;
			std::vector<SubmeshMetadataTemp> submeshData;
		};

	public:

		MeshTable(RenderDevice* rd, GPUGarbageBin* bin, const MemorySpecification& spec, bool async = false);

		// Mesh loaded to staging --> Will be uploaded to GPU by upload context! :)
		MeshContainerTemp LoadMesh(const MeshSpecification& spec, UploadContext& ctx);

		void FreeMesh(Mesh handle);

		// Get descriptor to attribute table
		u32 GetAttributeDescriptor(VertexAttributeTemp attr) const;

		// Get descriptor to submesh metadata table
		u32 GetSubmeshDescriptor() const;

		// Grab metadata for specific submesh on CPU
		const SubmeshMetadataTemp& GetSubmeshMD_CPU(Mesh mesh, u32 submesh);

		// Grab local offset for the submesh in the submesh table
		u32 GetSubmeshMD_GPU(Mesh mesh, u32 submesh);

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

		struct SubmeshHandle { u64 handle{ 0 }; friend class TypedHandlePool; };

		struct PositionHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct UVHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct NormalHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct TangentHandle { u64 handle{ 0 }; friend class TypedHandlePool; };

		struct Mesh_Storage
		{
			PositionHandle pos;
			UVHandle uv;
			NormalHandle nor;
			TangentHandle tan;

			std::vector<SubmeshMetadataTemp> mdsCpu;

			// Maybe only use one Handle here?
			// It is a base to the Mesh --> We can offset with the Submesh index passed!kl;
			std::vector<SubmeshHandle> mdsGpu;
		};

	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };

		HandleAllocator m_handleAtor;
		std::vector<std::optional<Mesh_Storage>> m_resources;

		// Submesh metadata for vertex pulling
		std::unique_ptr<GPUTableDeviceLocal<SubmeshHandle>> m_submeshTable;

		// Vertex attributes
		std::unique_ptr<GPUTableDeviceLocal<PositionHandle>> m_positionTable;
		std::unique_ptr<GPUTableDeviceLocal<UVHandle>> m_uvTable;
		std::unique_ptr<GPUTableDeviceLocal<NormalHandle>> m_normalTable;
		std::unique_ptr<GPUTableDeviceLocal<TangentHandle>> m_tangentTable;




	};
}