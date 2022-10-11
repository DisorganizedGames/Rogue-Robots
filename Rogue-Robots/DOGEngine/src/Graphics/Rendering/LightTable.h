#pragma once
#include "GPUTable.h"

namespace DOG::gfx
{
	struct LightHandle { friend TypedHandlePool; u64 handle{ 0 }; };
	enum class LightType
	{
		Point, Spot, Area
	};

	enum class LightUpdateFrequency
	{
		Never,		// Static light
		Sometimes,	// Dynamic (assumed less than once per frame on average)
		PerFrame	// Dynamic (once per frame)
	};

	struct PointLightDesc
	{
		
	};

	/*
		SpotlightComponent
		{
			SpotlightDesc desc;
			LightHandle handle;
		
		}
	
	*/

	struct SpotLightDesc
	{
		DirectX::SimpleMath::Vector3 position{ 0.f, 0.f, 0.f };
		DirectX::SimpleMath::Vector3 direction{ 0.f, 0.f, 1.f };
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		float strength{ 1.f };
	};

	struct AreaLightDesc
	{
		
	};

	class RenderDevice;
	class GPUGarbageBin;
	class UploadContext;

	class LightTable
	{
	public:
		struct PerTypeSpecification
		{
			u32 maxStatics{ 50 };
			u32 maxDynamic{ 10 };
			u32 maxSometimes{ 25 };

			u32 GetTotal() const { return maxStatics + maxDynamic + maxSometimes; }
		};

		struct StorageSpecification
		{
			PerTypeSpecification pointLightSpec;
			PerTypeSpecification spotLightSpec;
			PerTypeSpecification areaLightSpec;
		};

	public:
		LightTable(RenderDevice* rd, GPUGarbageBin* bin, const StorageSpecification& spec, bool async = false);
		
		// ======== User interface for declaring GPU lights
		LightHandle AddPointLight(const PointLightDesc& desc, LightUpdateFrequency frequency);
		LightHandle AddSpotLight(const SpotLightDesc& desc, LightUpdateFrequency frequency);
		LightHandle AddAreaLight(const AreaLightDesc& desc, LightUpdateFrequency frequency);

		void RemoveLight(LightHandle handle);

		// Update dynamic lights
		void UpdatePointLight(LightHandle handle, const PointLightDesc& desc);
		void UpdateSpotLight(LightHandle handle, const SpotLightDesc& desc);
		void UpdateAreaLight(LightHandle handle, const AreaLightDesc& desc);

		







		// ======== Implementation interface
		void SendCopyRequests(UploadContext& ctx);
		
		// Get array start per light type
		u32 GetDescriptor(LightType type);
		u32 GetChunkOffset(LightType type, LightUpdateFrequency freq);


	private:
		struct Light_Storage
		{
			LightType type{ LightType::Point };
			LightUpdateFrequency freq{ LightUpdateFrequency::Never };
			u32 localLightID{ UINT_MAX };
		};

		struct PointLight_GPUElement
		{
			DirectX::SimpleMath::Vector4 position{ 0.f, 0.f, 0.f, 1.f };
			DirectX::SimpleMath::Vector4 color{ 0.f, 0.f, 0.f, 1.f };
			DirectX::SimpleMath::Vector4 attenuation{ 0.f, 0.f, 0.f, 1.f };
		};

		struct SpotLight_GPUElement
		{
			DirectX::SimpleMath::Vector4 position{ 0.f, 0.f, 0.f, 1.f };
			DirectX::SimpleMath::Vector4 color{ 0.f, 0.f, 0.f, 0.f };
			DirectX::SimpleMath::Vector3 direction{ 0.f, 0.f, 1.f };
			float strength{ 1.f };
		};

		struct AreaLight_GPUElement
		{
			DirectX::SimpleMath::Vector4 position{ 0.f, 0.f, 0.f, 1.f };
		};

		struct LightMetadata_GPU
		{
			// Static light chunks
			u32 staticPointLightOffset;
			u32 staticSpotLightOffset;
			u32 staticAreaLightOffset;

			// Static light count
			u32 staticPointLightCount;
			u32 staticSpotLightCount;
			u32 staticAreaLightCount;

			// Dynamics
			u32 dynPointLightOffset;
			u32 dynPointLightCount;

			u32 dynSpotLightOffset;
			u32 dynSpotLightCount;

			u32 dynAreaLightOffset;
			u32 dynAreaLightCount;

			// Infreqs
			u32 infreqPointLightOffset;
			u32 infreqPointLightCount;

			u32 infreqSpotLightOffset;
			u32 infreqSpotLightCount;

			u32 infreqAreaLightOffset;
			u32 infreqAreaLightCount;
		};


	private:
		template <typename T>
		class PrivateStack
		{
		public:
			void push(T value)
			{
				const uint32_t placement = m_end++;

				// manual resizing if needed
				if (m_reusable_keys.size() < m_end)
					m_reusable_keys.resize(m_reusable_keys.size() * 2);

				m_reusable_keys[placement] = value;
			}

			T top()
			{
				assert(!empty());
				return m_reusable_keys[m_end - 1];
			}

			void pop()
			{
				assert(!empty());
				--m_end;
			}

			bool empty()
			{
				return m_end == 0;
			}
		private:
			std::vector<T> m_reusable_keys{ 0 };
			uint32_t m_end{ 0 };
		};

		template <typename Handle>
		struct LightMetadata
		{
			std::unique_ptr<GPUTableDeviceLocal<Handle>> bufferGPU;

			PrivateStack<u32> freeStatics, freeDynamics, freeInfreqs;

			u32 staticsCpuOffset{ 0 };
			u32 infreqsCpuOffset{ 0 };
			u32 dynamicsCpuOffset{ 0 };

			u32 numMaxStatics{ 0 };
			u32 numMaxInfreqs{ 0 };
			u32 numMaxDynamics{ 0 };

			bool staticsUploaded{ false };

			// Chunks per frequency : { handle, dirty } 
			Handle staticsHandle;
			std::pair<Handle, bool> dynamicsHandle;
			std::pair<Handle, bool> infreqsHandle;

		// === Helpers

			u32 GetNextSlot(LightUpdateFrequency freq)
			{
				u32 ret{ UINT_MAX };
				switch (freq)
				{
				case LightUpdateFrequency::Never:
					ret = freeStatics.top();
					freeStatics.pop();
					break;
				case LightUpdateFrequency::Sometimes:
					ret = freeInfreqs.top();
					freeInfreqs.pop();
					SetInfreqsChunkDirty(true);			// Assuming that retrieving a new slot means that new data is to be copied in
					break;
				case LightUpdateFrequency::PerFrame:
					ret = freeDynamics.top();
					freeDynamics.pop();
					SetDynamicsChunkDirty(true);		// See above
					break;
				}

				assert(ret != UINT_MAX);
				return ret;
			}
			void ReturnSlot(u32 slot, LightUpdateFrequency freq)
			{
				switch (freq)
				{
				case LightUpdateFrequency::Never:
					freeStatics.push(slot);
					break;
				case LightUpdateFrequency::Sometimes:
					freeInfreqs.push(slot);
					break;
				case LightUpdateFrequency::PerFrame:
					freeDynamics.push(slot);
					break;
				default:
					assert(false);
				}
			}

			bool DynamicsChunkDirty() const { return dynamicsHandle.second; }
			bool InfreqsChunkDirty() const { return infreqsHandle.second; }
			void SetDynamicsChunkDirty(bool dirty) { dynamicsHandle.second = dirty; }
			void SetInfreqsChunkDirty(bool dirty) { infreqsHandle.second = dirty; }
			
			void TryUpdateDynamics(void* chunkStart, u32 elementSize)
			{
				if (DynamicsChunkDirty())
				{
					bufferGPU->RequestUpdate(
						dynamicsHandle.first,
						chunkStart,
						numMaxDynamics * elementSize);
				}
			}
			void TryUpdateInfreqs(void* chunkStart, u32 elementSize)
			{
				if (InfreqsChunkDirty())
				{
					bufferGPU->RequestUpdate(
						infreqsHandle.first,
						chunkStart,
						numMaxInfreqs * elementSize);
				}
			}

			void UpdateStatics(void* chunkStart, u32 elementSize)
			{
				if (!staticsUploaded)
				{
					staticsUploaded = true;
					bufferGPU->RequestUpdate(
						staticsHandle,
						chunkStart,
						numMaxInfreqs * elementSize, 
						true);
				}
			}

			u32 GetGlobalDescriptor() const { return bufferGPU->GetGlobalDescriptor(); }
			u32 GetLocalChunkOffset(LightUpdateFrequency freq)
			{
				switch (freq)
				{
				case LightUpdateFrequency::Never:
					return bufferGPU->GetLocalOffset(staticsHandle);
				case LightUpdateFrequency::Sometimes:
					return bufferGPU->GetLocalOffset(infreqsHandle.first);
				case LightUpdateFrequency::PerFrame:
					return bufferGPU->GetLocalOffset(dynamicsHandle.first);
				default:
					assert(false);
				}
				return UINT_MAX;
			}

		};

	private:
		HandleAllocator m_handleAtor;
		StorageSpecification m_spec;
		std::vector<std::optional<Light_Storage>> m_lights;

		// CPU stored version of the light chunks
		std::vector<PointLight_GPUElement> m_pointLights;
		std::vector<SpotLight_GPUElement> m_spotLights;
		std::vector<AreaLight_GPUElement> m_areaLights;

		// GPU handles
		struct PointLightHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct SpotLightHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		struct AreaLightHandle { u64 handle{ 0 }; friend class TypedHandlePool; };

		// GPU table per light type
		LightMetadata<PointLightHandle> m_pointLightsMD;
		LightMetadata<SpotLightHandle> m_spotLightsMD;
		LightMetadata<AreaLightHandle> m_areaLightsMD;

		LightMetadata_GPU m_lightMD;
		struct LightMDHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		std::unique_ptr<GPUTableDeviceLocal<LightMDHandle>> m_lightsMD;
		LightMDHandle m_mdHandle;
	};


}