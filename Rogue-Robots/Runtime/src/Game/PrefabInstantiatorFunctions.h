#pragma once
#include <DOGEngine.h>

std::vector<DOG::entity> SpawnPlayers(const DirectX::SimpleMath::Vector3& pos, u8 playerCount, f32 spread = 10.f);

std::vector<DOG::entity> AddFlashlightsToPlayers(const std::vector<DOG::entity>& players);

DOG::entity SpawnTurretProjectile(const DirectX::SimpleMath::Matrix& transform, float speed, float dmg, float lifeTime, DOG::entity turret, DOG::entity owner);

DOG::entity SpawnLaserBlob(const DOG::TransformComponent& transform, DOG::entity owner) noexcept;