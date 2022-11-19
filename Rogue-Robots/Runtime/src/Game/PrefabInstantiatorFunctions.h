#pragma once
#include <DOGEngine.h>

std::vector<DOG::entity> SpawnPlayers(const DirectX::SimpleMath::Vector3& pos, u8 playerCount, f32 spread = 10.f);

std::vector<DOG::entity> AddFlashlightsToPlayers(const std::vector<DOG::entity>& players);

DOG::entity SpawnTurretProjectile(const DirectX::SimpleMath::Matrix& transform, float speed, DOG::entity turret);

DOG::entity CreateStaticPointLight(DirectX::SimpleMath::Vector3 pos, DirectX::SimpleMath::Vector3 color, float strength, float radius, bool debugSphereVisualizer = false);