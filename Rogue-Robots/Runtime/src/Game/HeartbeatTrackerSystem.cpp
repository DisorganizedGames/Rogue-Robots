#include "HeartbeatTrackerSystem.h"
#include "../DOGEngine/src/Graphics/Rendering/PostProcess.h"
#include "../DOGEngine/src/Core/Time.h" 
#include "GameComponent.h"

using namespace DOG;

HeartbeatTrackerSystem::HeartbeatTrackerSystem()
{
	m_heartbeatAudioEntity = EntityManager::Get().CreateEntity();
	AudioComponent& comp = EntityManager::Get().AddComponent<AudioComponent>(m_heartbeatAudioEntity);
	comp.loopStart = 0.0f;
	comp.loopEnd = -1.0f;
	comp.loop = true;
	comp.volume = 0.3f,

	m_heartbeatSound = AssetManager::Get().LoadAudio("Assets/Audio/PlayerHurt/LowHealth__L_Fixed.wav");
}

void HeartbeatTrackerSystem::OnUpdate(DOG::entity e, DOG::ThisPlayer& tp, PlayerStatsComponent& psc)
{
	UNREFERENCED_PARAMETER(e);
	UNREFERENCED_PARAMETER(tp);

	
	if (!DOG::EntityManager::Get().HasComponent<PlayerAliveComponent>(e))
	{
		m_justImpact = false;
		DOG::gfx::PostProcess::Get().SetHeartbeatFactor(0.f);
		DOG::gfx::PostProcess::Get().SetHeartbeatTransitionFactor(0.f);
		return;
	}

	AudioComponent& healthAudioComponent = EntityManager::Get().GetComponent<AudioComponent>(m_heartbeatAudioEntity);
	if (psc.health <= m_healthThreshold)
	{
		if (!healthAudioComponent.playing)
		{
			healthAudioComponent.assetID = m_heartbeatSound;
			healthAudioComponent.shouldPlay = true;
		}
		healthAudioComponent.volume = 0.3f;

		if (!m_justImpact)
		{
			m_justImpact = true;
			m_impactTime = (f32)DOG::Time::ElapsedTime();
		}

		f32 localElapsedNormalized = std::clamp((f32)(DOG::Time::ElapsedTime() - m_impactTime) / m_timeToStabilize, 0.f, 1.f);
		f32 intensity = m_impactIntensity * (1.f - localElapsedNormalized) + m_returnIntensity * (localElapsedNormalized);
		DOG::gfx::PostProcess::Get().SetHeartbeatFactor(intensity);

		f32 transitionFac = m_impactTransitionFactor * (1.f - localElapsedNormalized) + m_returnTransitionFactor * (localElapsedNormalized);
		DOG::gfx::PostProcess::Get().SetHeartbeatTransitionFactor(transitionFac);
	}
	else 
	{
		m_justImpact = false;
		DOG::gfx::PostProcess::Get().SetHeartbeatFactor(0.f);
		healthAudioComponent.volume = 0.0f;
	}
}
