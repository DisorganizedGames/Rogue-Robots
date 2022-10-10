#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class MainPlayer
{
	using Vector3 = DirectX::SimpleMath::Vector3;
public:
	MainPlayer();

public:
	void OnUpdate();

	const DOG::entity GetEntity() const noexcept;
private:
	DOG::EntityManager& m_entityManager;
	DOG::entity m_playerEntity;
	
	f32 m_azim, m_polar;
	Vector3 m_forward, m_right, m_up, m_position;
	f32 m_moveSpeed;
	inline const static Vector3 s_globalUp = Vector3(0, 1, 0);
public:
	bool m_moveView = true;
};