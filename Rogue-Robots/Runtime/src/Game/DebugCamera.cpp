#include "DebugCamera.h"

using namespace DirectX;

DebugCamera::DebugCamera()
{
	InitCamera(0, 0, 0);
}

DebugCamera::DebugCamera(f32 x, f32 y, f32 z)
{
	InitCamera(x, y, z);
}

void DebugCamera::OnUpdate()
{
	auto [mouseX, mouseY] = DOG::Mouse::GetDeltaCoordinates();
	m_polar += mouseX * 1.f/2000 * 2 * XM_PI;
	m_azim -= mouseY * 1.f/2000 * 2 * XM_PI;
	
	if (mouseX || mouseY)
	{
		Vector originalForward = XMVectorSet(0, 0, 1, 0);

		Vector quat = XMQuaternionRotationAxis(-m_right, m_azim);
		m_forward = XMVector3Rotate(originalForward, quat);

		quat = XMQuaternionRotationAxis(s_globalUp, m_polar);
		m_forward = XMVector3Rotate(m_forward, quat);
	}

	m_right = XMVector3Cross(s_globalUp, m_forward);
	Vector moveTowards = XMVectorSet(0, 0, 0, 0);

	if (DOG::Keyboard::IsKeyPressed(DOG::Key::W))
	{
		moveTowards += m_forward;
	}
	if (DOG::Keyboard::IsKeyPressed(DOG::Key::A))
	{
		moveTowards -= m_right;
	}
	if (DOG::Keyboard::IsKeyPressed(DOG::Key::S))
	{
		moveTowards -= m_forward;
	}
	if (DOG::Keyboard::IsKeyPressed(DOG::Key::D))
	{
		moveTowards += m_right;
	}
	
	Vector lengthVec = XMVector3Length(moveTowards);
	if (lengthVec.m128_f32[0] > 0.0001)
	{
		moveTowards = XMVector3Normalize(moveTowards);
		m_position += moveTowards * m_speed /* Time::DeltaTime */;
	}

	if (DOG::Keyboard::IsKeyPressed(DOG::Key::Spacebar))
	{
		m_position += s_globalUp * m_speed;
	}
	if (DOG::Keyboard::IsKeyPressed(DOG::Key::Shift))
	{
		m_position -= s_globalUp * m_speed;
	}

}

void DebugCamera::PrintPosition()
{
	f32 x = XMVectorGetX(m_position);
	f32 y = XMVectorGetY(m_position);
	f32 z = XMVectorGetZ(m_position);

	std::cout << "\x1b[H \x1b[?25l" << x << ", " << y << ", " << z << "                            " << std::endl;
	std::cout << m_polar << ", " << m_azim << "                            " << std::endl;
}

inline void DebugCamera::InitCamera(f32 x, f32 y, f32 z, f32 forwardX, f32 forwardY, f32 forwardZ)
{
	m_position = XMVectorSet(x, y, z, 0);
	m_forward = XMVectorSet(forwardX, forwardY, forwardZ, 0);
	m_right = XMVector3Cross(s_globalUp, m_forward);
	m_speed = 0.001;

	m_polar = m_azim = 0;
}
