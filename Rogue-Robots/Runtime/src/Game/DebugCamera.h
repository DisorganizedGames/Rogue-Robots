#pragma once
#include <DOGEngine.h>
#include <DirectXMath.h>

class DebugCamera
{
	using Vector = DirectX::XMVECTOR;
private:
	Vector m_position;
	Vector m_forward;
	Vector m_right;

	f32 m_polar, m_azim;

	f32 m_speed;

public:
	DebugCamera();
	DebugCamera(f32 x, f32 y, f32 z);
	~DebugCamera() = default;

public:
	void OnUpdate();

	// TODO: TEMPORARY
	void PrintPosition();

private:
	inline static Vector s_globalUp = DirectX::XMVectorSet(0, 1, 0, 0);

private:
	inline void InitCamera(f32 x, f32 y, f32 z, f32 forwardX=0, f32 forwardY=0, f32 forwardZ=1);

};