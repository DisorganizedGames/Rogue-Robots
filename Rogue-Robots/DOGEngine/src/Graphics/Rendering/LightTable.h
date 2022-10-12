#pragma once
#include "GPUTable.h"
#include "../../Core/Types/GraphicsTypes.h"

namespace DOG::gfx
{
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
		DirectX::SimpleMath::Vector3 position{ 0.f, 0.f, 0.f };
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		float strength{ 1.f };
	};

	struct SpotLightDesc
	{
		DirectX::SimpleMath::Vector3 position{ 0.f, 0.f, 0.f };
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		DirectX::SimpleMath::Vector3 direction{ 0.f, 0.f, 1.f };
		float strength{ 1.f };
		float cutoffAngle{ 15.f };
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
			u32 maxDynamic{ 30 };
			u32 maxSometimes{ 30 };

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
		void EnableLight(LightHandle handle);
		void DisableLight(LightHandle handle);

		// Update dynamic lights
		void UpdatePointLight(LightHandle handle, const PointLightDesc& desc);
		void UpdateSpotLight(LightHandle handle, const SpotLightDesc& desc);
		void UpdateAreaLight(LightHandle handle, const AreaLightDesc& desc);

		
		// ======== Implementation interface
		void FinalizeUpdates();
		void SendCopyRequests(UploadContext& ctx);
		
		// Get array start per light type
		u32 GetDescriptor(LightType type);
		u32 GetChunkOffset(LightType type, LightUpdateFrequency freq);
		u32 GetMetadataDescriptor();


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
			float strength{ 1.f };
		};

		struct SpotLight_GPUElement
		{
			DirectX::SimpleMath::Vector4 position{ 0.f, 0.f, 0.f, 1.f };
			DirectX::SimpleMath::Vector3 color{ 0.f, 0.f, 0.f };
			float cutoffAngle{ 15.f };
			DirectX::SimpleMath::Vector3 direction{ 0.f, 0.f, 1.f };
			float strength{ 1.f };
		};

		struct AreaLight_GPUElement
		{
			DirectX::SimpleMath::Vector4 position{ 0.f, 0.f, 0.f, 1.f };
		};

		struct LightMetadata_GPU
		{
			// Point lights (statics, infreqs, dynamics)
			std::pair<u32, u32> staticPointLightRange;
			std::pair<u32, u32> infreqPointLightRange;
			std::pair<u32, u32> dynPointLightRange;

			// Spot lights
			std::pair<u32, u32> staticSpotLightRange;
			std::pair<u32, u32> infreqSpotLightRange;
			std::pair<u32, u32> dynSpotLightRange;

			// Area lights
			std::pair<u32, u32> staticAreaLightRange;
			std::pair<u32, u32> infreqAreaLightRange;
			std::pair<u32, u32> dynAreaLightRange;
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
		struct PerFreqMetadata
		{
			PrivateStack<u32> freeSlots;
			std::pair<u32, u32> range{ 0, 0 };			// { offset, count }
			std::pair<Handle, bool> handle;				// { handle, dirty }

			void FillSlots()
			{
				assert(range.second != 0);
				for (i32 i = range.second - 1; i >= 0; --i)
					freeSlots.push(range.first + i);
			}
		};

		template <typename Handle>
		struct LightMetadata
		{
			std::unique_ptr<GPUTableDeviceLocal<Handle>> bufferGPU;
			PerFreqMetadata<Handle> statics, infreqs, dynamics;


		// === Helpers
			void FillSlots()
			{
				statics.FillSlots();
				infreqs.FillSlots();
				dynamics.FillSlots();
			}

			u32 GetNextSlot(LightUpdateFrequency freq)
			{
				u32 ret{ UINT_MAX };
				switch (freq)
				{
				case LightUpdateFrequency::Never:
					ret = statics.freeSlots.top();
					statics.freeSlots.pop();
					break;
				case LightUpdateFrequency::Sometimes:
					ret = infreqs.freeSlots.top();
					infreqs.freeSlots.pop();
					SetInfreqsChunkDirty(true);			// Assuming that retrieving a new slot means that new data is to be copied in
					break;
				case LightUpdateFrequency::PerFrame:
					ret = dynamics.freeSlots.top();
					dynamics.freeSlots.pop();
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
					SetStaticsChunkDirty(true);
					statics.freeSlots.push(slot);
					break;
				case LightUpdateFrequency::Sometimes:
					infreqs.freeSlots.push(slot);
					SetInfreqsChunkDirty(true);	
					break;
				case LightUpdateFrequency::PerFrame:
					dynamics.freeSlots.push(slot);
					SetDynamicsChunkDirty(true);
					break;
				default:
					assert(false);
				}
			}


			bool DynamicsChunkDirty() const { return dynamics.handle.second; }
			bool InfreqsChunkDirty() const { return infreqs.handle.second; }
			bool StaticsChunkDirty() const { return statics.handle.second; }

			void SetDynamicsChunkDirty(bool dirty) { dynamics.handle.second = dirty; }
			void SetInfreqsChunkDirty(bool dirty) { infreqs.handle.second = dirty; }
			void SetStaticsChunkDirty(bool dirty) { statics.handle.second = dirty; }
			
			void TryUpdateDynamics(void* chunkStart, u32 elementSize)
			{
				if (DynamicsChunkDirty())
				{
					bufferGPU->RequestUpdate(
						dynamics.handle.first,
						chunkStart,
						dynamics.range.second * elementSize);
				}
			}
			void TryUpdateInfreqs(void* chunkStart, u32 elementSize)
			{
				if (InfreqsChunkDirty())
				{
					bufferGPU->RequestUpdate(
						infreqs.handle.first,
						chunkStart,
						infreqs.range.second * elementSize);
				}
			}
			void TryUpdateStatics(void* chunkStart, u32 elementSize)
			{
				if (statics.handle.second)
				{
					statics.handle.second = false;
					bufferGPU->RequestUpdate(
						statics.handle.first,
						chunkStart,
						statics.range.second * elementSize, 
						true);
				}
			}

			u32 GetGlobalDescriptor() const { return bufferGPU->GetGlobalDescriptor(); }
			u32 GetLocalChunkOffset(LightUpdateFrequency freq)
			{
				switch (freq)
				{
				case LightUpdateFrequency::Never:
					return bufferGPU->GetLocalOffset(statics.handle.first);
				case LightUpdateFrequency::Sometimes:
					return bufferGPU->GetLocalOffset(infreqs.handle.first);
				case LightUpdateFrequency::PerFrame:
					return bufferGPU->GetLocalOffset(dynamics.handle.first);
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

		// Metadata
		struct LightMDHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
		std::unique_ptr<GPUTableDeviceLocal<LightMDHandle>> m_lightsMD;
		LightMetadata_GPU m_lightMD;
		LightMDHandle m_mdHandle;
	};


}