#include "MainPlayer.h"

using namespace DOG;
using namespace DirectX;

MainPlayer::MainPlayer() : m_entityManager(EntityManager::Get())
{
	//Camera
	m_playerEntity = m_entityManager.CreateEntity();
	CameraComponent& camera = m_entityManager.AddComponent<CameraComponent>(m_playerEntity);
	CameraComponent::s_mainCamera = &camera;

	m_right		= Vector3(1, 0, 0);
	m_up		= Vector3(0, 1, 0);
	m_forward	= Vector3(0, 0, 1);

	m_position	= Vector3(0, 0, 0);

	m_azim = XM_PI / 2;
	m_polar = XM_PI / 2;
	m_moveSpeed = 10.0f;

	UpdateCamera(camera);

	f32 aspectRatio = (f32)Window::GetWidth() / Window::GetHeight();
	camera.projMatrix = XMMatrixPerspectiveFovLH(80.f * XM_PI / 180.f, aspectRatio, 800.f, 0.1f);

	//Temporary ScriptManager until we have a global one.
	m_tempSM = std::make_unique<ScriptManager>(ScriptManager(&LuaW::s_luaW));
	//Gun
	GunComponent& gun = m_entityManager.AddComponent<GunComponent>(m_playerEntity);
	gun.gunScript = m_tempSM->AddScript("Gun.lua");

	//Temporary until we can call functions easily.
	LuaTable table(&LuaW::s_luaW, gun.gunScript->luaScript, true);
	table.CallFunctionOnTable(gun.gunScript->onStart);
}

void MainPlayer::OnUpdate()
{
	auto& cameraComp = m_entityManager.GetComponent<CameraComponent>(m_playerEntity);
	UpdateCamera(cameraComp);

	auto& gunComp = m_entityManager.GetComponent<GunComponent>(m_playerEntity);
	//Temporary until we can call functions easily.
	LuaTable table(&LuaW::s_luaW, gunComp.gunScript->luaScript, true);
	table.CallFunctionOnTable(gunComp.gunScript->onUpdate);
}

void MainPlayer::UpdateCamera(CameraComponent& camera)
{
	// Not using a structured binding to avoid getting screwed over by the layout of the CameraComponent changing
	auto& view = camera.viewMatrix;

	auto [mouseX, mouseY] = DOG::Mouse::GetDeltaCoordinates();

	m_azim -= mouseX * 1.f/2000 * 2 * XM_PI;
	m_polar += mouseY * 1.f/2000 * 2 * XM_PI;

	m_polar = (f32)std::min((double)m_polar, XM_PI - 0.001);
	m_polar = (f32)std::max((double)m_polar, 0.001);

	m_forward = XMVectorSet(
		std::cos(m_azim) * std::sin(m_polar),
		std::cos(m_polar),
		std::sin(m_azim) * std::sin(m_polar),
		0
	);

	m_right = s_globalUp.Cross(m_forward);
	Vector3 xzForward = m_forward;
	xzForward.y = 0;
	xzForward.Normalize();

	Vector3 moveTowards = Vector3(0, 0, 0);

	if (DOG::Keyboard::IsKeyPressed(DOG::Key::W))
	{
		moveTowards += xzForward;
	}
	if (DOG::Keyboard::IsKeyPressed(DOG::Key::A))
	{
		moveTowards -= m_right;
	}
	if (DOG::Keyboard::IsKeyPressed(DOG::Key::S))
	{
		moveTowards -= xzForward;
	}
	if (DOG::Keyboard::IsKeyPressed(DOG::Key::D))
	{
		moveTowards += m_right;
	}

	f32 lengthVec = moveTowards.Length();
	if (lengthVec > 0.0001)
	{
		moveTowards = XMVector3Normalize(moveTowards);
		m_position += moveTowards * m_moveSpeed * (f32)Time::DeltaTime();
	}

	if (DOG::Keyboard::IsKeyPressed(DOG::Key::Spacebar))
	{
		m_position += s_globalUp * m_moveSpeed * (f32)Time::DeltaTime();
	}
	if (DOG::Keyboard::IsKeyPressed(DOG::Key::Shift))
	{
		m_position -= s_globalUp * m_moveSpeed * (f32)Time::DeltaTime();
	}
	
	m_up = m_forward.Cross(m_right);

	view = XMMatrixLookToLH(m_position, m_forward, m_up);
}
