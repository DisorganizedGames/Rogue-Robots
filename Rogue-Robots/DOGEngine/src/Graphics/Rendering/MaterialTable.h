#pragma once
#include "GPUTable.h"
#include "../Handles/HandleAllocator.h"
#include "../../Core/Types/MaterialTypes.h"

namespace DOG::gfx
{
	class RenderDevice;
	class GPUGarbageBin;
	class UploadContext;

	class MaterialTable
	{
	public:
		struct MemorySpecification
		{
			u32 maxElements{ 0 };
		};

		struct MaterialSpecification
		{
			std::optional<TextureView> albedo, normal, metallicRoughness, emissive;

			/*
				other data to shader..
				other data to shader..
			*/
		};

	public:
		MaterialTable(RenderDevice* rd, GPUGarbageBin* bin, const MemorySpecification& spec, bool async = false);

		MaterialHandle LoadMaterial(const MaterialSpecification& spec, UploadContext& ctx);
		void FreeMaterial(MaterialHandle handle);

		void UpdateMaterial(MaterialHandle handle, const MaterialSpecification& spec);
		
		//void SendCopyRequests(UploadContext& ctx);

		u32 GetDescriptor() const { return m_matTable->GetGlobalDescriptor(); }
		u32 GetMaterialIndex(MaterialHandle handle) const;

	private:
		struct GPUMatHandle { u64 handle{ 0 }; friend class TypedHandlePool; };

		struct Material_GPUElement
		{
			u32 albedo{ 0 };
			u32 normal{ 0 };
			u32 metallicRoughness{ 0 };
			u32 emissive{ 0 };

			// other types to shader..
		};

		struct Material_Storage
		{
			GPUMatHandle gpuHandle;		
			Material_GPUElement gpu;	// keep latest cpu copy
		};

	private:
		void ConvertSpecToElement(const MaterialSpecification& spec, Material_GPUElement& gpuElement);


	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };

		HandleAllocator m_handleAtor;
		std::vector<std::optional<Material_Storage>> m_resources;

		std::unique_ptr<GPUTableDeviceLocal<GPUMatHandle>> m_matTable;
	};
}