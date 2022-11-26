#pragma once
#include <DOGEngine.h>

DOG::entity GetPlayer() noexcept;
DOG::entity GetGun() noexcept;

DOG::entity GetCamera() noexcept;
DOG::entity GetPlayersCamera(DOG::entity player) noexcept;