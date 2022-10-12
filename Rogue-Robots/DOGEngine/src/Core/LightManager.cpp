#include "LightManager.h"
#include "../Graphics/Rendering/Renderer.h"
#include "../Graphics/Rendering/LightTable.h"

namespace DOG
{
	LightManager* LightManager::s_instance = nullptr;

	void LightManager::Initialize(gfx::Renderer* renderer)
	{
		if (!s_instance)
			s_instance = new LightManager(renderer);
	}

	void LightManager::Destroy()
	{
		if (s_instance)
		{
			delete s_instance;
			s_instance = nullptr;
		}
	}

	LightManager& LightManager::Get()
	{
		assert(s_instance);
		return *s_instance;
	}

	LightHandle LightManager::AddPointLight(const PointLightDesc& desc, LightUpdateFrequency frequency)
	{
		return m_lightTable->AddPointLight(desc, frequency);
	}

	LightHandle LightManager::AddSpotLight(const SpotLightDesc& desc, LightUpdateFrequency frequency)
	{
		return m_lightTable->AddSpotLight(desc, frequency);
	}

	LightHandle LightManager::AddAreaLight(const AreaLightDesc& desc, LightUpdateFrequency frequency)
	{
		return m_lightTable->AddAreaLight(desc, frequency);
	}

	void LightManager::RemoveLight(LightHandle handle)
	{
		m_lightTable->RemoveLight(handle);
	}

	void LightManager::EnableLight(LightHandle handle)
	{
		m_lightTable->EnableLight(handle);
	}

	void LightManager::DisableLight(LightHandle handle)
	{
		m_lightTable->DisableLight(handle);
	}

	void LightManager::UpdatePointLight(LightHandle handle, const PointLightDesc& desc)
	{
		m_lightTable->UpdatePointLight(handle, desc);
	}

	void LightManager::UpdateSpotLight(LightHandle handle, const SpotLightDesc& desc)
	{
		m_lightTable->UpdateSpotLight(handle, desc);
	}

	void LightManager::UpdateAreaLight(LightHandle handle, const AreaLightDesc& desc)
	{
		m_lightTable->UpdateAreaLight(handle, desc);
	}

	LightManager::LightManager(gfx::Renderer* renderer) :
		m_lightTable(renderer->GetLightTable())
	{
		
	}

	LightManager::~LightManager()
	{
	}
}