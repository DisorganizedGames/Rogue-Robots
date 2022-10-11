#include "MainPlayer.h"

using namespace DOG;
using namespace DirectX;

MainPlayer::MainPlayer() : m_entityManager(EntityManager::Get())
{
	//Camera
	m_playerEntity = m_entityManager.CreateEntity();

	auto& psComp = m_entityManager.AddComponent<PlayerStatsComponent>(m_playerEntity);
	psComp = {
		.health = 100.f,
		.maxHealth = 100.f,
		.speed = 10.f
	};
	m_right		= Vector3(1, 0, 0);
	m_up		= Vector3(0, 1, 0);
	m_forward	= Vector3(0, 0, 1);

	m_position	= Vector3(0, 0, 0);

	m_azim = XM_PI / 2;
	m_polar = XM_PI / 2;
	m_moveSpeed = 10.0f;

}

void MainPlayer::OnUpdate()
{
	EntityManager::Get().Collect<InputController, CameraComponent, TransformComponent, RigidbodyComponent>()
		.Do([&](InputController& inputC, CameraComponent& cameraC, TransformComponent& transformC, RigidbodyComponent& rb)
		{
	
			f32 aspectRatio = (f32)Window::GetWidth() / Window::GetHeight();
			CameraComponent::s_mainCamera = &cameraC;
			cameraC.projMatrix = XMMatrixPerspectiveFovLH(80.f * XM_PI / 180.f, aspectRatio, 800.f, 0.1f);
			
			auto& view = cameraC.viewMatrix;
			auto speed = m_entityManager.GetComponent<PlayerStatsComponent>(m_playerEntity).speed;
			if (m_moveView)
			{
				auto [mouseX, mouseY] = DOG::Mouse::GetDeltaCoordinates();

				m_azim -= mouseX * 1.f / 2000 * 2 * XM_PI;
				m_polar += mouseY * 1.f / 2000 * 2 * XM_PI;

				m_polar = (f32)std::min((double)m_polar, XM_PI - 0.001);
				m_polar = (f32)std::max((double)m_polar, 0.001);

				m_forward = XMVectorSet(
					std::cos(m_azim) * std::sin(m_polar),
					std::cos(m_polar),
					std::sin(m_azim) * std::sin(m_polar),
					0
				);
			}

			m_right = s_globalUp.Cross(m_forward);
			Vector3 xzForward = m_forward;
			xzForward.y = 0;
			xzForward.Normalize();

			Vector3 moveTowards = Vector3(0, 0, 0);

			if (inputC.forward)
			{
				moveTowards += xzForward;
			}
			if (inputC.left)
			{
				moveTowards -= m_right;
			}
			if (inputC.backwards)
			{
				moveTowards -= xzForward;
			}
			if (inputC.right)
			{
				moveTowards += m_right;
			}

			f32 lengthVec = moveTowards.Length();
			moveTowards = XMVector3Normalize(moveTowards);
			rb.linearVelocity.x = moveTowards.x * speed;
			rb.linearVelocity.z = moveTowards.z * speed;
			//transformC.SetPosition((transformC.GetPosition() += moveTowards * speed * (f32)Time::DeltaTime()));
			
			if (inputC.up)
			{
				rb.linearVelocity.y = speed;
			}
			if (inputC.down)
			{
				//transformC.SetPosition(transformC.GetPosition() -= s_globalUp * speed * (f32)Time::DeltaTime());
			}

			m_up = m_forward.Cross(m_right);

			view = XMMatrixLookToLH(transformC.GetPosition(), m_forward, m_up);
			transformC.worldMatrix = view.Invert();
		});

}


const DOG::entity MainPlayer::GetEntity() const noexcept
{
	return m_playerEntity;
}
