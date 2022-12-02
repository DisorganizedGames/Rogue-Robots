#pragma once

#include <DOGEngine.h>
#include "GameComponent.h"

class SpectatorCopyCamera
{
public:
	static void CollectAndUpdate();

private:
	static void OnUpdate(DOG::entity e, PlayerControllerComponent& player);
};