#include "PostProcess.h"

using namespace DirectX::SimpleMath;

namespace DOG::gfx
{
	PostProcess* PostProcess::s_instance = nullptr;

	void PostProcess::Initialize()
	{
		if (!s_instance)
			s_instance = new PostProcess();
	}

	void PostProcess::Destroy()
	{
		if (s_instance)
		{
			delete s_instance;
			s_instance = nullptr;
		}
	}

	PostProcess& PostProcess::Get()
	{
		assert(s_instance != nullptr);
		return *s_instance;
	}

	PostProcess::PostProcess()
	{
		m_damageDiskDatas.resize(50);
	}

	void PostProcess::Update(f32 dt)
	{
		m_elapsedTime += dt;
		
	
		for (auto& ddd : m_damageDiskDatas)
		{
			f32 localElapsed = m_elapsedTime - ddd.currElapsed;
			f32 normalizedTime = std::clamp(localElapsed / ddd.timeDelta, 0.f, 1.f);
			f32 targetIntensity = std::clamp(ddd.startIntensity - 0.5f, 0.f, 1.f);
			ddd.currIntensity = ddd.startIntensity * (1.f - normalizedTime) + targetIntensity * normalizedTime;
			ddd.visibility = 1.f - normalizedTime;

			auto vec4 = DirectX::SimpleMath::Vector4(ddd.initDir2D.x, 0.f, ddd.initDir2D.y, 0.f);
			auto rotated = vec4.Transform(vec4, m_viewMat);
			ddd.dir2D = { rotated.x, rotated.z };
		}

		// heartbeat
		f32 iTime = m_elapsedTime;
		m_heartbeatIntensity = cos(iTime * 5.f) * 0.5f + 0.5f;
		m_heartbeatIntensity *= m_heartbeatFactor;
	}

	void PostProcess::SetViewMat(const DirectX::SimpleMath::Matrix& viewMat)
	{
		m_viewMat = viewMat;
	}


	void PostProcess::InstantiateDamageDisk(const DirectX::SimpleMath::Vector2& dir, f32 startIntensity, f32 timeToDisappear)
	{
		DamageDiskData ddd;

		ddd.currElapsed = m_elapsedTime;
		ddd.dir2D = dir;
		ddd.initDir2D = dir;
		ddd.timeDelta = timeToDisappear;
		ddd.startIntensity = startIntensity;
		ddd.visibility = 1.f;

		while (!(m_damageDiskDatas[m_nextDisk].visibility < 0.01f))
			m_nextDisk = (m_nextDisk + 1) % 50;

		m_damageDiskDatas[m_nextDisk] = ddd;
	}

	void PostProcess::InstantiateLaserBeam(Vector3 startPos, Vector3 endPos, Vector3 up, Vector3 color)
	{
		DirectX::SimpleMath::Matrix m = DirectX::SimpleMath::Matrix::CreateLookAt(startPos, endPos, up);
		m.Invert(m);
		m_laserBeams.emplace_back(m, color, DirectX::SimpleMath::Vector3::Distance(startPos, endPos));
	}

}