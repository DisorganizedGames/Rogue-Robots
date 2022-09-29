#include "MaterialTable.h"
#include "UploadContext.h"
#include "GPUGarbageBin.h"
#include "../RHI/RenderDevice.h"

namespace DOG::gfx
{
	MaterialTable::MaterialTable(RenderDevice* rd, GPUGarbageBin* bin, const MemorySpecification& spec, bool async) :
		m_rd(rd),
		m_bin(bin)
	{
		m_resources.resize(1);

		assert(spec.maxElements != 0);
		m_matTable = std::make_unique<GPUTableDeviceLocal<GPUMatHandle>>(rd, bin, (u32)sizeof(Material_GPUElement), spec.maxElements, async);
	}

	MaterialHandle MaterialTable::LoadMaterial(const MaterialSpecification& spec, UploadContext& ctx)
	{
		Material_Storage storage{};

		ConvertSpecToElement(spec, storage.gpu);

		storage.gpuHandle = m_matTable->Allocate(1, (void*)&storage.gpu);
		m_matTable->SendCopyRequests(ctx);

		// reserve handle and storage
		auto hdl = m_handleAtor.Allocate<MaterialHandle>();
		HandleAllocator::TryInsert(m_resources, storage, HandleAllocator::GetSlot(hdl.handle));
		
		return hdl;
	}

	void MaterialTable::FreeMaterial(MaterialHandle handle)
	{
		const auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));

		// Push for deferred deletion
		m_matTable->Free(res.gpuHandle);

		// Invalidate immediately (CPU access from here on assumed to not occur)
		HandleAllocator::FreeStorage(m_handleAtor, m_resources, handle);
	}

	void MaterialTable::UpdateMaterial(MaterialHandle handle, const MaterialSpecification& spec)
	{
		// Free old
		auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));
		m_matTable->Free(res.gpuHandle);
			
		// Allocate new
		ConvertSpecToElement(spec, res.gpu);
		res.gpuHandle = m_matTable->Allocate(1, &res.gpu);
	}

	//void MaterialTable::SendCopyRequests(UploadContext& ctx)
	//{
	//	m_matTable->SendCopyRequests(ctx);
	//}

	u32 MaterialTable::GetMaterialIndex(MaterialHandle handle) const
	{
		const auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));

		return m_matTable->GetLocalOffset(res.gpuHandle);
	}

	void MaterialTable::ConvertSpecToElement(const MaterialSpecification& spec, Material_GPUElement& gpuElement)
	{
		auto load = [this](u32& dest, std::optional<TextureView> view)
		{
			if (view)
				dest = m_rd->GetGlobalDescriptor(*view);
			else
			{
				dest = UINT32_MAX;
				// dest m_rd->GetGlobalDescriptor(m_defaultTextures->GetDefaultAlbedo());
			}
		};

		load(gpuElement.albedo, spec.albedo);
		load(gpuElement.normal, spec.normal);
		load(gpuElement.metallicRoughness, spec.metallicRoughness);
		load(gpuElement.emissive, spec.emissive);

		gpuElement.albedoFactor = spec.albedoFactor;
		gpuElement.emissiveFactor = spec.emissiveFactor;
		gpuElement.metallicFactor = spec.metallicFactor;
		gpuElement.roughnessFactor = spec.roughnessFactor;
	}



}