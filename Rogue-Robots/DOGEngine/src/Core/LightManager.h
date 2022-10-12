#pragma once
#include "../ECS/Component.h"

namespace DOG
{
	namespace gfx
	{
		class Renderer;
		class LightTable;
	}

	class LightManager
	{
	public:
		static void Initialize(gfx::Renderer* renderer);
		static void Destroy();
		static LightManager& Get();

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

	private:
		LightManager(gfx::Renderer* renderer);
		~LightManager();

	private:
		static LightManager* s_instance;
		gfx::LightTable* m_lightTable;


	};
}