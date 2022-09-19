#include "MeshTable.h"
#include "../RHI/RenderDevice.h"
#include "GPUGarbageBin.h"
#include "UploadContext.h"

namespace DOG::gfx
{
	MeshTable::MeshTable(RenderDevice* rd, GPUGarbageBin* bin, const MemorySpecification& spec, bool async) :
		m_rd(rd),
		m_bin(bin)
	{
		m_resources.resize(1);

		assert(spec.maxTotalSubmeshes != 0);
		m_submeshTable = std::make_unique<GPUTableDeviceLocal<SubmeshHandle>>(rd, bin, (u32)sizeof(SubmeshMetadata), spec.maxTotalSubmeshes, async);

		// Assumes indices always exist
		m_indexTable = std::make_unique<GPUTableDeviceLocal<IndexHandle>>(rd, bin, (u32)sizeof(u32), spec.maxNumIndices, async);

		// Assumes position always exist
		{
			u32 maxElements = spec.maxSizePerAttribute.find(VertexAttribute::Position)->second / POS_STRIDE;
			m_positionTable = std::make_unique<GPUTableDeviceLocal<PositionHandle>>(rd, bin, POS_STRIDE, maxElements, async);
		}

		// Assumes UV always exist
		{
			u32 maxElements = spec.maxSizePerAttribute.find(VertexAttribute::UV)->second / UV_STRIDE;
			m_uvTable = std::make_unique<GPUTableDeviceLocal<UVHandle>>(rd, bin, UV_STRIDE, maxElements, async);
		}

		// Assumes Normal always exist
		{
			u32 maxElements = spec.maxSizePerAttribute.find(VertexAttribute::Normal)->second / NOR_STRIDE;
			m_normalTable = std::make_unique<GPUTableDeviceLocal<NormalHandle>>(rd, bin, NOR_STRIDE, maxElements, async);
		}

		// Assumes tangents always exist
		{
			u32 maxElements = spec.maxSizePerAttribute.find(VertexAttribute::Tangent)->second / TAN_STRIDE;
			m_tangentTable = std::make_unique<GPUTableDeviceLocal<TangentHandle>>(rd, bin, TAN_STRIDE, maxElements, async);
		}

	}

	MeshContainer MeshTable::LoadMesh(const MeshSpecification& spec, UploadContext& ctx)
	{
		Mesh_Storage storage{};

		// Load submeshes ==> Use a single handle for submeshes and use submesh as a local offset
		for (const auto& md : spec.submeshData)
		{
			storage.mdsCpu.push_back(md);
		}
		storage.mdsGpu = m_submeshTable->Allocate((u32)spec.submeshData.size(), (void*)spec.submeshData.data());
		m_submeshTable->SendCopyRequests(ctx);

		auto loadAttrData = [this](const MeshSpecification& spec, VertexAttribute attr) -> std::span<u8>
		{
			return spec.vertexDataPerAttribute.find(attr)->second;
		};

		// Load indices
		{
			storage.idx = m_indexTable->Allocate((u32)spec.indices.size(), spec.indices.data());
			m_indexTable->SendCopyRequests(ctx);
		}

		// Load position
		{
			const auto& attrData = loadAttrData(spec, VertexAttribute::Position);
			u32 numElements = (u32)attrData.size_bytes() / POS_STRIDE;
			storage.pos = m_positionTable->Allocate(numElements, attrData.data());
			m_positionTable->SendCopyRequests(ctx);
		}

		// Load UV
		{
			const auto& attrData = loadAttrData(spec, VertexAttribute::UV);
			u32 numElements = (u32)attrData.size_bytes() / UV_STRIDE;
			storage.uv = m_uvTable->Allocate(numElements, attrData.data());
			m_uvTable->SendCopyRequests(ctx);
		}

		// Load nor
		{
			const auto& attrData = loadAttrData(spec, VertexAttribute::Normal);
			u32 numElements = (u32)attrData.size_bytes() / NOR_STRIDE;
			storage.nor = m_normalTable->Allocate(numElements, attrData.data());
			m_normalTable->SendCopyRequests(ctx);
		}

		// Load tangent
		{
			const auto& attrData = loadAttrData(spec, VertexAttribute::Tangent);
			u32 numElements = (u32)attrData.size_bytes() / TAN_STRIDE;
			storage.tan = m_tangentTable->Allocate(numElements, attrData.data());
			m_tangentTable->SendCopyRequests(ctx);
		}

		auto handle = m_handleAtor.Allocate<Mesh>();
		HandleAllocator::TryInsert(m_resources, storage, HandleAllocator::GetSlot(handle.handle));

		MeshContainer container{};
		container.mesh = handle;
		container.numSubmeshes = (u32)spec.submeshData.size();

		return container;
	}

	void MeshTable::FreeMesh(Mesh handle)
	{
		auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));

		// Push safe deletion
		auto delFunc = [this,
			pos = res.pos,
			uv = res.uv,
			nor = res.nor,
			tan = res.tan,
			mds = std::move(res.mdsGpu)		// We are moving the vector! No deep copies
		]() mutable
		{
			m_positionTable->Free(pos);
			m_uvTable->Free(uv);
			m_normalTable->Free(nor);
			m_tangentTable->Free(tan);
			m_submeshTable->Free(mds);
		};
		m_bin->PushDeferredDeletion(delFunc);

		// Invalidate handle
		HandleAllocator::FreeStorage(m_handleAtor, m_resources, handle);
	}

	u32 MeshTable::GetAttributeDescriptor(VertexAttribute attr) const
	{
		switch (attr)
		{
		case VertexAttribute::Position:
			return m_positionTable->GetGlobalDescriptor();
			break;
		case VertexAttribute::UV:
			return m_uvTable->GetGlobalDescriptor();
			break;
		case VertexAttribute::Normal:
			return m_normalTable->GetGlobalDescriptor();
			break;
		case VertexAttribute::Tangent:
			return m_tangentTable->GetGlobalDescriptor();
			break;

		default:
			assert(false);
		}
		return (u32)-1;
	}

	u32 MeshTable::GetSubmeshDescriptor() const
	{
		return m_submeshTable->GetGlobalDescriptor();
	}

	const SubmeshMetadata& MeshTable::GetSubmeshMD_CPU(Mesh mesh, u32 submesh)
	{
		const auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(mesh.handle));
		assert(submesh < res.mdsCpu.size());

		return res.mdsCpu[submesh];
	}

	u32 MeshTable::GetSubmeshMD_GPU(Mesh mesh, u32 submesh)
	{
		const auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(mesh.handle));

		u32 localOffset = submesh;
		// Get to mesh, then use mesh-local offset to find submesh
		return m_submeshTable->GetLocalOffset(res.mdsGpu) + localOffset;
	}

	Buffer MeshTable::GetIndexBuffer()
	{
		return m_indexTable->GetBuffer();
	}
}