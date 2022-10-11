#include "LightTable.h"
#include "UploadContext.h"

namespace DOG::gfx
{
	LightTable::LightTable(RenderDevice* rd, GPUGarbageBin* bin, const StorageSpecification& spec, bool async) :
		m_spec(spec)
	{
		m_lights.resize(1);

		const u32 maxPointLights = spec.pointLightSpec.GetTotal();
		const u32 maxSpotLights = spec.spotLightSpec.GetTotal();
		const u32 maxAreaLights = spec.areaLightSpec.GetTotal();

		m_pointLights.resize(maxPointLights);
		m_spotLights.resize(maxSpotLights);
		m_areaLights.resize(maxAreaLights);

		m_pointLightsMD.bufferGPU = std::make_unique<GPUTableDeviceLocal<PointLightHandle>>(rd, bin, (u32)sizeof(PointLight_GPUElement), maxPointLights * (bin->GetMaxVersions() + 1), async);
		m_spotLightsMD.bufferGPU = std::make_unique<GPUTableDeviceLocal<SpotLightHandle>>(rd, bin, (u32)sizeof(SpotLight_GPUElement), maxSpotLights * (bin->GetMaxVersions() + 1), async);
		m_areaLightsMD.bufferGPU = std::make_unique<GPUTableDeviceLocal<AreaLightHandle>>(rd, bin, (u32)sizeof(AreaLight_GPUElement), maxAreaLights * (bin->GetMaxVersions() + 1), async);
		m_lightsMD = std::make_unique<GPUTableDeviceLocal<LightMDHandle>>(rd, bin, (u32)sizeof(LightMetadata_GPU), bin->GetMaxVersions() + 1, async);

		// Define light chunks
		// [ Statics, Infreqs, Dynamics ]
		{
			m_pointLightsMD.statics.range = { 0, spec.pointLightSpec.maxStatics };
			m_pointLightsMD.infreqs.range = { spec.pointLightSpec.maxStatics, spec.pointLightSpec.maxSometimes };
			m_pointLightsMD.dynamics.range = { spec.pointLightSpec.maxStatics + spec.pointLightSpec.maxSometimes, spec.pointLightSpec.maxDynamic };

			m_spotLightsMD.statics.range = { 0, spec.spotLightSpec.maxStatics };
			m_spotLightsMD.infreqs.range = { spec.spotLightSpec.maxStatics, spec.spotLightSpec.maxSometimes };
			m_spotLightsMD.dynamics.range = { spec.spotLightSpec.maxStatics + spec.spotLightSpec.maxSometimes, spec.spotLightSpec.maxDynamic };

			m_areaLightsMD.statics.range = { 0, spec.areaLightSpec.maxStatics };
			m_areaLightsMD.infreqs.range = { spec.areaLightSpec.maxStatics, spec.areaLightSpec.maxSometimes };
			m_areaLightsMD.dynamics.range = { spec.areaLightSpec.maxStatics + spec.areaLightSpec.maxSometimes, spec.areaLightSpec.maxDynamic };

			m_pointLightsMD.FillSlots();
			m_spotLightsMD.FillSlots();
			m_areaLightsMD.FillSlots();
		}

		// Setup MD Buffer
		{
			m_lightMD.staticPointLightRange = m_pointLightsMD.statics.range;
			m_lightMD.infreqPointLightRange = m_pointLightsMD.infreqs.range;
			m_lightMD.dynPointLightRange = m_pointLightsMD.dynamics.range;

m_lightMD.staticSpotLightRange = m_spotLightsMD.statics.range;
m_lightMD.infreqSpotLightRange = m_spotLightsMD.infreqs.range;
m_lightMD.dynSpotLightRange = m_spotLightsMD.dynamics.range;

m_lightMD.staticAreaLightRange = m_areaLightsMD.statics.range;
m_lightMD.infreqAreaLightRange = m_areaLightsMD.infreqs.range;
m_lightMD.dynAreaLightRange = m_areaLightsMD.dynamics.range;

m_mdHandle = m_lightsMD->Allocate(1, &m_lightMD);
		}

		// Allocate chunks
		{
		// Point lights
		m_pointLightsMD.statics.handle = { m_pointLightsMD.bufferGPU->Allocate(spec.pointLightSpec.maxStatics), true };
		m_pointLightsMD.dynamics.handle = { m_pointLightsMD.bufferGPU->Allocate(spec.pointLightSpec.maxDynamic), true };
		m_pointLightsMD.infreqs.handle = { m_pointLightsMD.bufferGPU->Allocate(spec.pointLightSpec.maxSometimes), true };

		// Spot lights
		m_spotLightsMD.statics.handle = { m_spotLightsMD.bufferGPU->Allocate(spec.spotLightSpec.maxStatics), true };
		m_spotLightsMD.dynamics.handle = { m_spotLightsMD.bufferGPU->Allocate(spec.spotLightSpec.maxDynamic), true };
		m_spotLightsMD.infreqs.handle = { m_spotLightsMD.bufferGPU->Allocate(spec.spotLightSpec.maxSometimes), true };

		// Area lights
		m_areaLightsMD.statics.handle = { m_areaLightsMD.bufferGPU->Allocate(spec.areaLightSpec.maxStatics), true };
		m_areaLightsMD.dynamics.handle = { m_areaLightsMD.bufferGPU->Allocate(spec.areaLightSpec.maxDynamic), true };
		m_areaLightsMD.infreqs.handle = { m_areaLightsMD.bufferGPU->Allocate(spec.areaLightSpec.maxSometimes), true };
		}
	}

	void LightTable::RemoveLight(LightHandle handle)
	{
		auto& storage = HandleAllocator::TryGet(m_lights, HandleAllocator::GetSlot(handle.handle));
		switch (storage.type)
		{
		case LightType::Point:
			std::memset(&m_pointLights[storage.localLightID], 0, sizeof(PointLight_GPUElement));
			m_pointLightsMD.ReturnSlot(storage.localLightID, storage.freq);
			break;
		case LightType::Spot:
			std::memset(&m_spotLights[storage.localLightID], 0, sizeof(SpotLight_GPUElement));
			m_spotLightsMD.ReturnSlot(storage.localLightID, storage.freq);
			break;
		case LightType::Area:
			std::memset(&m_areaLights[storage.localLightID], 0, sizeof(AreaLight_GPUElement));
			m_areaLightsMD.ReturnSlot(storage.localLightID, storage.freq);
			break;
		}

		HandleAllocator::FreeStorage(m_handleAtor, m_lights, handle);
	}

	void LightTable::UpdateSpotLight(LightHandle handle, const SpotLightDesc& desc)
	{
		const auto& storage = HandleAllocator::TryGet(m_lights, HandleAllocator::GetSlot(handle.handle));
		assert(storage.freq != LightUpdateFrequency::Never);
		assert(storage.type == LightType::Spot);

		auto& gpu = m_spotLights[storage.localLightID];
		gpu.position = DirectX::SimpleMath::Vector4(desc.position.x, desc.position.y, desc.position.z, 1.f);
		gpu.color = DirectX::SimpleMath::Vector4(desc.color.x, desc.color.y, desc.color.z, 1.f);
		gpu.direction = desc.direction;
		gpu.strength = desc.strength;

		switch (storage.freq)
		{
		case LightUpdateFrequency::Sometimes:
			m_spotLightsMD.SetInfreqsChunkDirty(true);
			break;
		case LightUpdateFrequency::PerFrame:
			m_spotLightsMD.SetDynamicsChunkDirty(true);
			break;
		}
	}


	LightHandle LightTable::AddSpotLight(const SpotLightDesc& desc, LightUpdateFrequency frequency)
	{
		// Grab slot
		u32 nextIdx = m_spotLightsMD.GetNextSlot(frequency);

		// Copy data
		auto& gpu = m_spotLights[nextIdx];
		gpu.position = DirectX::SimpleMath::Vector4(desc.position.x, desc.position.y, desc.position.z, 1.f);
		gpu.color = DirectX::SimpleMath::Vector4(desc.color.x, desc.color.y, desc.color.z, 1.f);
		gpu.direction = desc.direction;
		gpu.strength = desc.strength;

		// Store
		const auto lightHandle = m_handleAtor.Allocate<LightHandle>();
		Light_Storage storage{};
		storage.localLightID = nextIdx;
		storage.type = LightType::Spot;
		storage.freq = frequency;

		HandleAllocator::TryInsertMove(m_lights, std::move(storage), HandleAllocator::GetSlot(lightHandle.handle));
		return lightHandle;
	}

	void LightTable::FinalizeUpdates()
	{
		// If dirty --> Update GPU structures
		m_pointLightsMD.TryUpdateStatics(&m_pointLights[m_pointLightsMD.statics.range.first], sizeof(PointLight_GPUElement));
		m_pointLightsMD.TryUpdateInfreqs(&m_pointLights[m_pointLightsMD.infreqs.range.first], sizeof(PointLight_GPUElement));
		m_pointLightsMD.TryUpdateDynamics(&m_pointLights[m_pointLightsMD.dynamics.range.first], sizeof(PointLight_GPUElement));

		m_spotLightsMD.TryUpdateStatics(&m_spotLights[m_spotLightsMD.statics.range.first], sizeof(SpotLight_GPUElement));
		m_spotLightsMD.TryUpdateInfreqs(&m_spotLights[m_spotLightsMD.infreqs.range.first], sizeof(SpotLight_GPUElement));
		m_spotLightsMD.TryUpdateDynamics(&m_spotLights[m_spotLightsMD.dynamics.range.first], sizeof(SpotLight_GPUElement));

		m_areaLightsMD.TryUpdateStatics(&m_areaLights[m_areaLightsMD.statics.range.first], sizeof(AreaLight_GPUElement));
		m_areaLightsMD.TryUpdateInfreqs(&m_areaLights[m_areaLightsMD.infreqs.range.first], sizeof(AreaLight_GPUElement));
		m_areaLightsMD.TryUpdateDynamics(&m_areaLights[m_areaLightsMD.dynamics.range.first], sizeof(AreaLight_GPUElement));
	}

	void LightTable::SendCopyRequests(UploadContext& ctx)
	{
		// MD sent onces
		m_lightsMD->SendCopyRequests(ctx);

		if (m_pointLightsMD.DynamicsChunkDirty() || m_pointLightsMD.InfreqsChunkDirty())
			m_pointLightsMD.bufferGPU->SendCopyRequests(ctx);

		if (m_spotLightsMD.DynamicsChunkDirty() || m_spotLightsMD.InfreqsChunkDirty())
			m_spotLightsMD.bufferGPU->SendCopyRequests(ctx);

		if (m_areaLightsMD.DynamicsChunkDirty() || m_areaLightsMD.InfreqsChunkDirty())
			m_areaLightsMD.bufferGPU->SendCopyRequests(ctx);

		// Reset
		m_pointLightsMD.SetDynamicsChunkDirty(false);
		m_pointLightsMD.SetInfreqsChunkDirty(false);

		m_spotLightsMD.SetDynamicsChunkDirty(false);
		m_spotLightsMD.SetInfreqsChunkDirty(false);

		m_areaLightsMD.SetDynamicsChunkDirty(false);
		m_areaLightsMD.SetInfreqsChunkDirty(false);
	}

	u32 LightTable::GetDescriptor(LightType type)
	{
		switch (type)
		{
		case LightType::Point:
			return m_pointLightsMD.GetGlobalDescriptor();
		case LightType::Spot:
			return m_spotLightsMD.GetGlobalDescriptor();
		case LightType::Area:
			return m_areaLightsMD.GetGlobalDescriptor();
		default:
			assert(false);
			return 0;
		}
	}

	u32 LightTable::GetChunkOffset(LightType type, LightUpdateFrequency freq)
	{
		switch (type)
		{
		case LightType::Point:
			return m_pointLightsMD.GetLocalChunkOffset(freq);
		case LightType::Spot:
			return m_spotLightsMD.GetLocalChunkOffset(freq);
		case LightType::Area:
			return m_areaLightsMD.GetLocalChunkOffset(freq);
		default:
			break;
		}
	
		assert(false);
		return 0;
	}
	u32 LightTable::GetMetadataDescriptor()
	{
		return m_lightsMD->GetGlobalDescriptor();
	}


}