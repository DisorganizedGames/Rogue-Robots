#pragma once
#include <DOGEngine.h>

DOG::entity GetPlayer() noexcept;
DOG::entity GetGun() noexcept;

// The camera that gets rendered from.
DOG::entity GetCamera() noexcept;

// The player characters first person view camera. No guarantee that this is the main camera for that player, they might be spectating or using debug camera.
DOG::entity GetPlayerFPSCamera(DOG::entity player) noexcept;