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
	
	if (mouseX || mouseY)
	{
		m_azim -= mouseX * 1.f/2000 * 2 * XM_PI;
		m_polar += mouseY * 1.f/2000 * 2 * XM_PI;
		
		m_polar = std::min((double)m_polar, XM_PI - 0.001);
		m_polar = std::max((double)m_polar, 0.001);

		m_forward = XMVectorSet(
			std::cos(m_azim) * std::sin(m_polar),
			std::cos(m_polar), 
			std::sin(m_azim) * std::sin(m_polar),
			0
		);

		m_right = XMVector3Cross(s_globalUp, m_forward);
		m_viewDirty = true;
	}

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
		m_viewDirty = true;
	}

	if (DOG::Keyboard::IsKeyPressed(DOG::Key::Spacebar))
	{
		m_position += s_globalUp * m_speed;
		m_viewDirty = true;
	}
	if (DOG::Keyboard::IsKeyPressed(DOG::Key::Shift))
	{
		m_position -= s_globalUp * m_speed;
		m_viewDirty = true;
	}

}

DebugCamera::Matrix DebugCamera::GetViewMatrix()
{
	if (m_viewDirty)
	{
		GenerateViewMatrix();
		m_viewDirty = false;
	}
	return m_viewMatrix;
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
	m_speed = 0.1;

	m_polar = XM_PI / 2;
	m_azim = XM_PI / 2;
}

inline void DebugCamera::GenerateViewMatrix()
{
	Vector up = XMVector3Cross(m_forward, m_right);
	m_viewMatrix = XMMatrixLookToLH(m_position, m_forward, up);
}
