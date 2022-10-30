#pragma once
#include "GPUTable.h"
#include "../Handles/HandleAllocator.h"
#include "../../Core/Types/GraphicsTypes.h"

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

			// Change yourself if you want to support more data
			// Also change GPU Element struct and ConvertSpecToElement function to supply it
			DirectX::SimpleMath::Vector4 albedoFactor{ 1.f, 1.f, 1.f, 1.f };
			DirectX::SimpleMath::Vector4 emissiveFactor{ 0.f, 0.f, 0.f, 1.f };
			f32 metallicFactor{ 0.f };
			f32 roughnessFactor{ 1.f };

			f32 alpha{ 1.f };
		};

	public:
		MaterialTable(RenderDevice* rd, GPUGarbageBin* bin, const MemorySpecification& spec, bool async = false);

		MaterialHandle LoadMaterial(const MaterialSpecification& spec);
		void FreeMaterial(MaterialHandle handle);

		void UpdateMaterial(MaterialHandle handle, const MaterialSpecification& spec);
		
		void SendCopyRequests(UploadContext& ctx);

		u32 GetDescriptor() const { return m_matTable->GetGlobalDescriptor(); }
		u32 GetMaterialIndex(MaterialHandle handle) const;

	private:
		struct GPUMatHandle { u64 handle{ 0 }; friend class TypedHandlePool; };

		struct Material_GPUElement
		{
			u32 albedo{ 0 };
			u32 metallicRoughness{ 0 };
			u32 normal{ 0 };
			u32 emissive{ 0 };

			DirectX::SimpleMath::Vector4 albedoFactor{ 1.f, 1.f, 1.f, 1.f };
			DirectX::SimpleMath::Vector4 emissiveFactor{ 0.f, 0.f, 0.f, 1.f };
			f32 metallicFactor{ 0.f };
			f32 roughnessFactor{ 1.f };
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
		u32 m_resCapacity{ 0 };
		bool m_resCapacityChanged{ false };

		std::unique_ptr<GPUTableDeviceLocal<GPUMatHandle>> m_matTable;
	};
}