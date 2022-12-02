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
	void SpawnParticleSystem();

	template<class T>
	inline void SwitchToComponent();

	template<class T>
	inline void SwitchToBehaviorComponent();

	void ConeSettings();
	void CylinderSettings();
	void BoxSettings();

	void GravityOptions();
	void GravityPointOptions();
	void GravityDirectionOptions();
	void ConstVelocityOptions();
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

template<>
inline void ParticleScene::SwitchToBehaviorComponent<nullptr_t>()
{
	auto& em = DOG::EntityManager::Get();
	em.RemoveComponentIfExists<DOG::GravityBehaviorComponent>(m_particleSystem);
	em.RemoveComponentIfExists<DOG::NoGravityBehaviorComponent>(m_particleSystem);
	em.RemoveComponentIfExists<DOG::GravityPointBehaviorComponent>(m_particleSystem);
	em.RemoveComponentIfExists<DOG::GravityDirectionBehaviorComponent>(m_particleSystem);
	em.RemoveComponentIfExists<DOG::ConstVelocityBehaviorComponent>(m_particleSystem);
}

template<class T>
inline void ParticleScene::SwitchToBehaviorComponent()
{
	auto& em = DOG::EntityManager::Get();
	em.RemoveComponentIfExists<DOG::GravityBehaviorComponent>(m_particleSystem);
	em.RemoveComponentIfExists<DOG::NoGravityBehaviorComponent>(m_particleSystem);
	em.RemoveComponentIfExists<DOG::GravityPointBehaviorComponent>(m_particleSystem);
	em.RemoveComponentIfExists<DOG::GravityDirectionBehaviorComponent>(m_particleSystem);
	em.RemoveComponentIfExists<DOG::ConstVelocityBehaviorComponent>(m_particleSystem);

	AddComponent<T>(m_particleSystem);
}

