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

		{
			// cpu-side chunks are stored as such:
			// [ statics, infreqs, dynamics ] 
			// Offsets
			m_pointLightsMD.staticsCpuOffset = 0;
			m_pointLightsMD.infreqsCpuOffset = spec.pointLightSpec.maxStatics;
			m_pointLightsMD.dynamicsCpuOffset = spec.pointLightSpec.maxStatics + spec.pointLightSpec.maxSometimes;

			m_pointLightsMD.numMaxStatics = spec.pointLightSpec.maxStatics;
			m_pointLightsMD.numMaxInfreqs = spec.pointLightSpec.maxSometimes;
			m_pointLightsMD.numMaxDynamics = spec.pointLightSpec.maxDynamic;

			m_spotLightsMD.staticsCpuOffset = 0;
			m_spotLightsMD.infreqsCpuOffset = spec.spotLightSpec.maxStatics;
			m_spotLightsMD.dynamicsCpuOffset = spec.spotLightSpec.maxStatics + spec.spotLightSpec.maxSometimes;

			// Counts
			m_spotLightsMD.numMaxStatics = spec.spotLightSpec.maxStatics;
			m_spotLightsMD.numMaxInfreqs = spec.spotLightSpec.maxSometimes;
			m_spotLightsMD.numMaxDynamics = spec.spotLightSpec.maxDynamic;

			m_areaLightsMD.staticsCpuOffset = 0;
			m_areaLightsMD.infreqsCpuOffset = spec.areaLightSpec.maxStatics;
			m_areaLightsMD.dynamicsCpuOffset = spec.areaLightSpec.maxStatics + spec.areaLightSpec.maxSometimes;

			m_areaLightsMD.numMaxStatics = spec.areaLightSpec.maxStatics;
			m_areaLightsMD.numMaxInfreqs = spec.areaLightSpec.maxSometimes;
			m_areaLightsMD.numMaxDynamics = spec.areaLightSpec.maxDynamic;
		}


		// Setup MD Buffer
		{
			m_lightMD.staticPointLightOffset = m_pointLightsMD.staticsCpuOffset;
			m_lightMD.infreqPointLightOffset = m_pointLightsMD.infreqsCpuOffset;
			m_lightMD.dynPointLightOffset = m_pointLightsMD.dynamicsCpuOffset;

			m_lightMD.staticSpotLightOffset = m_spotLightsMD.staticsCpuOffset;
			m_lightMD.infreqSpotLightOffset = m_spotLightsMD.infreqsCpuOffset;
			m_lightMD.dynSpotLightOffset = m_spotLightsMD.dynamicsCpuOffset;

			m_lightMD.staticAreaLightOffset = m_areaLightsMD.staticsCpuOffset;
			m_lightMD.infreqAreaLightOffset = m_areaLightsMD.infreqsCpuOffset;
			m_lightMD.dynAreaLightOffset = m_areaLightsMD.dynamicsCpuOffset;

			// Counts
			m_lightMD.staticPointLightCount = m_pointLightsMD.numMaxStatics;
			m_lightMD.infreqPointLightCount = m_pointLightsMD.numMaxInfreqs;
			m_lightMD.dynPointLightCount = m_pointLightsMD.numMaxDynamics;

			m_lightMD.staticSpotLightCount = m_spotLightsMD.numMaxStatics;
			m_lightMD.infreqSpotLightCount = m_spotLightsMD.numMaxInfreqs;
			m_lightMD.dynSpotLightCount = m_spotLightsMD.numMaxDynamics;

			m_lightMD.staticAreaLightCount = m_spotLightsMD.numMaxStatics;
			m_lightMD.infreqAreaLightCount = m_spotLightsMD.numMaxInfreqs;
			m_lightMD.dynAreaLightCount = m_spotLightsMD.numMaxDynamics;
		}





		// =======

		// Point lights
		m_pointLightsMD.staticsHandle = m_pointLightsMD.bufferGPU->Allocate(spec.pointLightSpec.maxStatics);
		m_pointLightsMD.dynamicsHandle = { m_pointLightsMD.bufferGPU->Allocate(spec.pointLightSpec.maxDynamic), true };
		m_pointLightsMD.infreqsHandle = { m_pointLightsMD.bufferGPU->Allocate(spec.pointLightSpec.maxSometimes), true };

		// Spot lights
		m_spotLightsMD.staticsHandle = m_spotLightsMD.bufferGPU->Allocate(spec.spotLightSpec.maxStatics);
		m_spotLightsMD.dynamicsHandle = { m_spotLightsMD.bufferGPU->Allocate(spec.spotLightSpec.maxDynamic), true };
		m_spotLightsMD.infreqsHandle = { m_spotLightsMD.bufferGPU->Allocate(spec.spotLightSpec.maxSometimes), true };

		// Area lights
		m_areaLightsMD.staticsHandle = m_areaLightsMD.bufferGPU->Allocate(spec.areaLightSpec.maxStatics);
		m_areaLightsMD.dynamicsHandle = { m_areaLightsMD.bufferGPU->Allocate(spec.areaLightSpec.maxDynamic), true };
		m_areaLightsMD.infreqsHandle = { m_areaLightsMD.bufferGPU->Allocate(spec.areaLightSpec.maxSometimes), true };

		// Reserve slots
		const auto fillQueue = [](PrivateStack<u32>& stack, u32 numSlots, u32 offset)
		{
			for (i32 i = numSlots - 1; i >= 0; --i)
				stack.push(offset + (u32)i);
		};

		fillQueue(m_pointLightsMD.freeStatics, spec.pointLightSpec.maxStatics, m_pointLightsMD.staticsCpuOffset);
		fillQueue(m_pointLightsMD.freeInfreqs, spec.pointLightSpec.maxSometimes, m_pointLightsMD.infreqsCpuOffset);
		fillQueue(m_pointLightsMD.freeDynamics, spec.pointLightSpec.maxDynamic, m_pointLightsMD.dynamicsCpuOffset);

		fillQueue(m_spotLightsMD.freeStatics, spec.spotLightSpec.maxStatics, m_spotLightsMD.staticsCpuOffset);
		fillQueue(m_spotLightsMD.freeInfreqs, spec.spotLightSpec.maxSometimes, m_spotLightsMD.infreqsCpuOffset);
		fillQueue(m_spotLightsMD.freeDynamics, spec.spotLightSpec.maxDynamic, m_spotLightsMD.dynamicsCpuOffset);
	

		fillQueue(m_areaLightsMD.freeStatics, spec.areaLightSpec.maxStatics, m_areaLightsMD.staticsCpuOffset);
		fillQueue(m_areaLightsMD.freeInfreqs, spec.areaLightSpec.maxSometimes, m_areaLightsMD.infreqsCpuOffset);
		fillQueue(m_areaLightsMD.freeDynamics, spec.areaLightSpec.maxDynamic, m_areaLightsMD.dynamicsCpuOffset);
	
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

	void LightTable::UpdatePointLight(LightHandle handle, const PointLightDesc& desc)
	{
		assert(false);
	}

	void LightTable::UpdateSpotLight(LightHandle handle, const SpotLightDesc& desc)
	{
		const auto& storage = HandleAllocator::TryGet(m_lights, HandleAllocator::GetSlot(handle.handle));
		assert(storage.freq != LightUpdateFrequency::Never);
		
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

	void LightTable::UpdateAreaLight(LightHandle handle, const AreaLightDesc& desc)
	{
		assert(false);
	}

	LightHandle LightTable::AddPointLight(const PointLightDesc&, LightUpdateFrequency frequency)
	{
		assert(false);

		// Grab slot
		u32 nextIdx = m_pointLightsMD.GetNextSlot(frequency);
		
		// Copy data
		//auto& gpu = m_pointLights[nextIdx];


		// @todo fill data



		// Store
		const auto lightHandle = m_handleAtor.Allocate<LightHandle>();
		Light_Storage storage{};
		storage.localLightID = nextIdx;
		storage.type = LightType::Point;
		storage.freq = frequency;

		HandleAllocator::TryInsertMove(m_lights, std::move(storage), HandleAllocator::GetSlot(lightHandle.handle));
		return lightHandle;
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

	LightHandle LightTable::AddAreaLight(const AreaLightDesc& desc, LightUpdateFrequency frequency)
	{
		assert(false);
		return LightHandle();

	}

	void LightTable::SendCopyRequests(UploadContext& ctx)
	{
		m_lightsMD->RequestUpdate(m_mdHandle, &m_lightMD, sizeof(m_lightMD));
		m_lightsMD->SendCopyRequests(ctx);

		// If dirty --> Update GPU structures
		m_pointLightsMD.UpdateStatics(&m_pointLights[m_pointLightsMD.staticsCpuOffset], sizeof(PointLight_GPUElement));
		m_pointLightsMD.TryUpdateInfreqs(&m_pointLights[m_pointLightsMD.infreqsCpuOffset], sizeof(PointLight_GPUElement));
		m_pointLightsMD.TryUpdateDynamics(&m_pointLights[m_pointLightsMD.dynamicsCpuOffset], sizeof(PointLight_GPUElement));

		m_spotLightsMD.UpdateStatics(&m_spotLights[m_spotLightsMD.staticsCpuOffset], sizeof(SpotLight_GPUElement));
		m_spotLightsMD.TryUpdateInfreqs(&m_spotLights[m_spotLightsMD.infreqsCpuOffset], sizeof(SpotLight_GPUElement));
		m_spotLightsMD.TryUpdateDynamics(&m_spotLights[m_spotLightsMD.dynamicsCpuOffset], sizeof(SpotLight_GPUElement));

		m_areaLightsMD.UpdateStatics(&m_areaLights[m_areaLightsMD.staticsCpuOffset], sizeof(AreaLight_GPUElement));
		m_areaLightsMD.TryUpdateInfreqs(&m_areaLights[m_areaLightsMD.infreqsCpuOffset], sizeof(AreaLight_GPUElement));
		m_areaLightsMD.TryUpdateDynamics(&m_areaLights[m_areaLightsMD.dynamicsCpuOffset], sizeof(AreaLight_GPUElement));

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

}