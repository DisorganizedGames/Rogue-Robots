#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class HeartbeatTrackerSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::ThisPlayer, PlayerStatsComponent);
	ON_UPDATE_ID(DOG::ThisPlayer, PlayerStatsComponent);
	void OnUpdate(DOG::entity e, DOG::ThisPlayer& tp, PlayerStatsComponent& psc);

private:
	f32 m_returnIntensity{ 1.5f };		// Intensity to stabilize at when low HP
	f32 m_impactIntensity{ 10.0f };		// Intensity when hitting low HP threshold

	f32 m_returnTransitionFactor{ -0.39f };		// Narrower
	f32 m_impactTransitionFactor{ -0.35f };		// Wider

	f32 m_timeToStabilize{ 5.f };		// Time until intensity has stabilized from impactIntensity to returnIntensity

	bool m_justImpact{ false };
	f32 m_impactTime{ 0.f };
};