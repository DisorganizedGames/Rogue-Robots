#include "GoalRadarSystem.h"
#include "../DOGEngine/src/Core/Time.h" 
#include "GameComponent.h"

void GoalRadarSystem::OnUpdate(DOG::entity e, GoalRadarComponent& grc)
{
	auto& em = DOG::EntityManager::Get();
	m_radarElapsedTime += (f32)DOG::Time::DeltaTime();
	m_radarMaxTime = grc.timeVisible;

	// Turn off
	if (m_radarOn && m_radarElapsedTime >= m_radarMaxTime)
	{
		m_radarOn = false;
		m_radarElapsedTime = 0.f;
		m_radarMaxTime = 0.f;
		em.Collect<ExitBlockComponent>().Do([&](DOG::entity model, ExitBlockComponent&)
			{
				em.RemoveComponentIfExists<DOG::OutlineComponent>(model);
			});

		// Disable goal radar
		em.RemoveComponent<GoalRadarComponent>(e);
		return;
	}

	// Turn on if off
	if (!m_radarOn)
	{
		m_radarOn = true;
		em.Collect<ExitBlockComponent>().Do([&](DOG::entity model, ExitBlockComponent&)
			{
				if (em.HasComponent<DOG::OutlineComponent>(model))
					return;

				em.AddComponent<DOG::OutlineComponent>(model, DirectX::SimpleMath::Vector3{ 1.f, 0.f, 1.f });
			});
	}
}
