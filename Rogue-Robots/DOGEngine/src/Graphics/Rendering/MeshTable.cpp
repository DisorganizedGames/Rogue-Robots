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

		m_submeshTable = std::make_unique<GPUTableDeviceLocal<SubmeshHandle>>(rd, bin, (u32)sizeof(SubmeshMetadataTemp), spec.maxTotalSubmeshes, async);

		// Assumes position always exist
		{
			u32 maxElements = spec.maxSizePerAttribute.find(VertexAttributeTemp::Position)->second / POS_STRIDE;
			m_positionTable = std::make_unique<GPUTableDeviceLocal<PositionHandle>>(rd, bin, POS_STRIDE, maxElements, async);
		}

		// Assumes UV always exist
		{
			u32 maxElements = spec.maxSizePerAttribute.find(VertexAttributeTemp::UV)->second / UV_STRIDE;
			m_uvTable = std::make_unique<GPUTableDeviceLocal<UVHandle>>(rd, bin, UV_STRIDE, maxElements, async);
		}

		// Assumes Normal always exist
		{
			u32 maxElements = spec.maxSizePerAttribute.find(VertexAttributeTemp::Normal)->second / NOR_STRIDE;
			m_normalTable = std::make_unique<GPUTableDeviceLocal<NormalHandle>>(rd, bin, NOR_STRIDE, maxElements, async);
		}

		// Assumes tangents always exist
		{
			u32 maxElements = spec.maxSizePerAttribute.find(VertexAttributeTemp::Tangent)->second / TAN_STRIDE;
			m_tangentTable = std::make_unique<GPUTableDeviceLocal<TangentHandle>>(rd, bin, TAN_STRIDE, maxElements, async);
		}

	}

	MeshContainerTemp MeshTable::LoadMesh(const MeshSpecification& spec, UploadContext& ctx)
	{
		Mesh_Storage storage{};


		// Load submeshes ==> Use a single handle for submeshes and use submesh as a local offset
		for (const auto& md : spec.submeshData)
		{
			storage.mdsCpu.push_back(md);
		}
		m_submeshTable->Allocate((u32)spec.submeshData.size(), (void*)spec.submeshData.data());

		auto loadAttrData = [this](const MeshSpecification& spec, VertexAttributeTemp attr) -> std::span<u8>
		{
			return spec.vertexDataPerAttribute.find(attr)->second;
		};

		// Load position
		{
			const auto& attrData = loadAttrData(spec, VertexAttributeTemp::Position);
			u32 numElements = (u32)attrData.size_bytes() / POS_STRIDE;
			storage.pos = m_positionTable->Allocate(numElements, attrData.data());
		}

		// Load UV
		{
			const auto& attrData = loadAttrData(spec, VertexAttributeTemp::UV);
			u32 numElements = (u32)attrData.size_bytes() / UV_STRIDE;
			storage.uv = m_uvTable->Allocate(numElements, attrData.data());
		}

		// Load nor
		{
			const auto& attrData = loadAttrData(spec, VertexAttributeTemp::Normal);
			u32 numElements = (u32)attrData.size_bytes() / NOR_STRIDE;
			storage.nor = m_normalTable->Allocate(numElements, attrData.data());
		}

		// Load tangent
		{
			const auto& attrData = loadAttrData(spec, VertexAttributeTemp::Tangent);
			u32 numElements = (u32)attrData.size_bytes() / TAN_STRIDE;
			storage.tan = m_tangentTable->Allocate(numElements, attrData.data());
		}

		auto handle = m_handleAtor.Allocate<Mesh>();

		return MeshContainerTemp();
	}

	void MeshTable::FreeMesh(Mesh handle)
	{
		const auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));

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
			for (const auto& md : mds)
				m_submeshTable->Free(md);
		};
		m_bin->PushDeferredDeletion(delFunc);

		// Invalidate handle
		HandleAllocator::FreeStorage(m_handleAtor, m_resources, handle);
	}

	u32 MeshTable::GetAttributeDescriptor(VertexAttributeTemp attr) const
	{
		switch (attr)
		{
		case VertexAttributeTemp::Position:
			m_positionTable->GetGlobalDescriptor();
			break;
		case VertexAttributeTemp::UV:
			m_uvTable->GetGlobalDescriptor();
			break;
		case VertexAttributeTemp::Normal:
			m_normalTable->GetGlobalDescriptor();
			break;
		case VertexAttributeTemp::Tangent:
			m_tangentTable->GetGlobalDescriptor();
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

	const SubmeshMetadataTemp& MeshTable::GetSubmeshMD_CPU(Mesh mesh, u32 submesh)
	{
		const auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(mesh.handle));
		assert(submesh < res.mdsCpu.size());

		return res.mdsCpu[submesh];
	}

	u32 MeshTable::GetSubmeshMD_GPU(Mesh mesh, u32 submesh)
	{
		const auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(mesh.handle));
		assert(submesh < res.mdsGpu.size());

		u32 localOffset = submesh;
		return m_submeshTable->GetLocalOffset(res.mdsGpu[submesh]) + localOffset;
	}
}