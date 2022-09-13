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
		assert(spec.maxElements != 0);
		m_matTable = std::make_unique<GPUTableDeviceLocal<GPUMatHandle>>(rd, bin, sizeof(Material_GPUElement), spec.maxElements, async);
	}

	MaterialHandle MaterialTable::LoadMaterial(const MaterialSpecification& spec)
	{
		Material_Storage storage{};

		ConvertSpecToElement(spec, storage.gpu);

		// load to buffer
		storage.gpuHandle = m_matTable->Allocate(1, &storage.gpu);

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

	void MaterialTable::SendCopyRequests(UploadContext& ctx)
	{
		m_matTable->SendCopyRequests(ctx);
	}

	void MaterialTable::ConvertSpecToElement(const MaterialSpecification& spec, Material_GPUElement& gpuElement)
	{
		auto load = [this](u32& dest, std::optional<TextureView> view)
		{
			if (view)
				dest = m_rd->GetGlobalDescriptor(*view);
			else
			{
				// dest m_rd->GetGlobalDescriptor(m_defaultTextures->GetDefaultAlbedo());
			}
		};

		load(gpuElement.normal, spec.normal);
		load(gpuElement.metallicRoughness, spec.metallicRoughness);
		load(gpuElement.emissive, spec.emissive);
	}



}