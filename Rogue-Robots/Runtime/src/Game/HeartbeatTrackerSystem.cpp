#include "HeartbeatTrackerSystem.h"
#include "../DOGEngine/src/Graphics/Rendering/PostProcess.h"
#include "../DOGEngine/src/Core/Time.h"


void HeartbeatTrackerSystem::OnUpdate(DOG::entity e, DOG::ThisPlayer& tp, PlayerStatsComponent& psc)
{
	if (psc.health <= 20.f)
	{
		if (!m_justImpact)
		{
			m_justImpact = true;
			m_impactTime = (f32)DOG::Time::ElapsedTime();
		}

		f32 localElapsedNormalized = std::clamp((f32)(DOG::Time::ElapsedTime() - m_impactTime) / m_timeToStabilize, 0.f, 1.f);
		f32 intensity = m_impactIntensity * (1.f - localElapsedNormalized) + m_returnIntensity * (localElapsedNormalized);

		DOG::gfx::PostProcess::Get().SetHeartbeatFactor(intensity);
	}
	else 
	{
		m_justImpact = false;
		DOG::gfx::PostProcess::Get().SetHeartbeatFactor(0.f);
	}
}
