#pragma once
#include <DOGEngine.h>
#include "../Scene.h"

class ParticleScene : public Scene
{
	using ConeSpawnComponent = DOG::ConeSpawnComponent;
	using CylinderSpawnComponent = DOG::CylinderSpawnComponent;
	using BoxSpawnComponent = DOG::BoxSpawnComponent;

public:
	ParticleScene();
	~ParticleScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

private:
	DOG::entity m_particleSystem = DOG::NULL_ENTITY;

private:
	void ParticleSystemMenu(bool& open);

	template<class T>
	inline void SwitchToComponent();

	void ConeSettings();
	void CylinderSettings();
	void BoxSettings();
};

// Template specialization to switch back to default
template<>
inline void ParticleScene::SwitchToComponent<nullptr_t>()
{
	auto& em = DOG::EntityManager::Get();
	em.RemoveComponentIfExists<ConeSpawnComponent>(m_particleSystem);
	em.RemoveComponentIfExists<CylinderSpawnComponent>(m_particleSystem);
	em.RemoveComponentIfExists<BoxSpawnComponent>(m_particleSystem);
}

template<class T>
inline void ParticleScene::SwitchToComponent()
{
	auto& em = DOG::EntityManager::Get();
	em.RemoveComponentIfExists<ConeSpawnComponent>(m_particleSystem);
	em.RemoveComponentIfExists<CylinderSpawnComponent>(m_particleSystem);
	em.RemoveComponentIfExists<BoxSpawnComponent>(m_particleSystem);

	AddComponent<T>(m_particleSystem);
}

