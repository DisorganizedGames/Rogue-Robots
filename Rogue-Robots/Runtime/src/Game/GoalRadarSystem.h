#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class GoalRadarSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(GoalRadarComponent);
	ON_UPDATE_ID(GoalRadarComponent);
	void OnUpdate(DOG::entity e, GoalRadarComponent& grc);

private:
	// You can change the max time of the goal radar in ActiveItems.lua. The variable is at the top
	f32 m_radarMaxTime{ 0.f };			// Set statically, but from Lua

	f32 m_radarElapsedTime{ 0.f };
	bool m_radarOn{ false };

};