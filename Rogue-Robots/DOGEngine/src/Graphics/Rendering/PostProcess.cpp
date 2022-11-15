#include "PostProcess.h"

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
		
		// Simulate Disk data		
		//{
		//	f32 localElapsed = m_elapsedTime - m_damageDiskData.currElapsed;
		//	f32 normalizedTime = std::clamp(localElapsed / m_damageDiskData.timeDelta, 0.f, 1.f);
		//	f32 targetIntensity = std::clamp(m_damageDiskData.startIntensity - 0.5f, 0.f, 1.f);
		//	m_damageDiskData.currIntensity = m_damageDiskData.startIntensity * (1.f - normalizedTime) + targetIntensity * normalizedTime;

		//	m_damageDiskData.visibility = 1.f - normalizedTime;
		//}
	
		for (auto& ddd : m_damageDiskDatas)
		{
			f32 localElapsed = m_elapsedTime - ddd.currElapsed;
			f32 normalizedTime = std::clamp(localElapsed / ddd.timeDelta, 0.f, 1.f);
			f32 targetIntensity = std::clamp(ddd.startIntensity - 0.5f, 0.f, 1.f);
			ddd.currIntensity = ddd.startIntensity * (1.f - normalizedTime) + targetIntensity * normalizedTime;
			ddd.visibility = 1.f - normalizedTime;
		}
	}

	void PostProcess::InstantiateDamageDisk(const DirectX::SimpleMath::Vector2& dir, f32 startIntensity, f32 timeToDisappear)
	{
		DamageDiskData ddd;

		ddd.currElapsed = m_elapsedTime;
		ddd.dir2D = dir;
		ddd.timeDelta = timeToDisappear;
		ddd.startIntensity = startIntensity;
		ddd.visibility = 1.f;

		m_damageDiskData = ddd;

		while (!(m_damageDiskDatas[m_nextDisk].visibility < 0.01f))
			m_nextDisk = (m_nextDisk + 1) % 50;

		m_damageDiskDatas[m_nextDisk] = ddd;
	}



}