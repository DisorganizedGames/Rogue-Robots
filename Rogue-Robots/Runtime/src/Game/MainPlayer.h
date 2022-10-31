#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class MainPlayer
{
	using Vector3 = DirectX::SimpleMath::Vector3;
public:
	MainPlayer();
	~MainPlayer();

public:
	void OnUpdate();
	void ForceDebugCamera(bool value);
private:
	DOG::EntityManager& m_entityManager;

	bool m_useDebugView = false;
	DOG::entity m_debugCamera;
	
	f32 m_azim, m_polar;
	Vector3 m_forward, m_right, m_up;
	f32 m_moveSpeed;
	inline const static Vector3 s_globalUp = Vector3(0, 1, 0);
public:
	bool m_moveView = true;
	bool m_forceDebugCamera = false;
};