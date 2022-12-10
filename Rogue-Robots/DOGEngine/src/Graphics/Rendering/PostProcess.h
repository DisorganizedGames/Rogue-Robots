#pragma once
#include <vector>

namespace DOG::gfx
{
	/*
		Global data center for post process data that needs to be instantiated
	*/

	class PostProcess
	{
	public:
		struct DamageDiskData
		{
			DirectX::SimpleMath::Vector2 dir2D, initDir2D;

			// Interpolate from startIntensity --> 0 over 'timeToDisappear'
			f32 startIntensity{ 0.f };

			f32 currElapsed{ 0.f };			// Elapsed time on instantiation
			f32 timeDelta{ 0.f };			// Difference between current time and time + timeToDisappear

			f32 currIntensity{ 0.f };
			f32 visibility{ 0.f };

			DirectX::SimpleMath::Vector3 color;
		};

		struct LaserBeamData
		{
			DirectX::SimpleMath::Matrix matrix;
			DirectX::SimpleMath::Vector3 color;
			f32 length;
		};

		struct ShockWaveData
		{
			DirectX::SimpleMath::Vector3 position;
			DirectX::SimpleMath::Vector3 args;
			f32 lifeTime;
			f32 currentTime;
		};

	private:
		PostProcess();

		void Update(f32 dt);
		void Clear();
		friend class Renderer;


	public:
		static void Initialize();
		static void Destroy();
		static PostProcess& Get();

		void SetViewMat(const DirectX::SimpleMath::Matrix& viewMat);
		void SetProjMat(const DirectX::SimpleMath::Matrix& projMat);

		// Damage bow
		void InstantiateDamageDisk(const DirectX::SimpleMath::Vector2& dir, f32 startIntensity, f32 timeToDisappear, const DirectX::SimpleMath::Vector3& color = { 1.f, 0.f, 0.f });
		void InstantiateLaserBeam(DirectX::SimpleMath::Vector3 startPos, DirectX::SimpleMath::Vector3 endPos, DirectX::SimpleMath::Vector3 up, DirectX::SimpleMath::Vector3 color);
		void InstantiateShockWave(DirectX::SimpleMath::Vector3 position, DirectX::SimpleMath::Vector3 args, f32 lifeTIme);

		// Heartbeat	
		void SetHeartbeatFactor(f32 factor) { m_heartbeatFactor = factor; }
		void SetHeartbeatTransitionFactor(f32 factor) { m_heartbeatTransitionFactor = factor; }

		// For rendering
		const std::vector<DamageDiskData>& GetDamageDisks() const { return m_damageDiskDatas; }
		f32 GetHeartbeatIntensity() const { return m_heartbeatIntensity; }
		f32 GetHeartbeatTransitionFactor() const { return m_heartbeatTransitionFactor; }
		std::vector<LaserBeamData>& GetLaserBeams() { return m_laserBeams; }
		std::vector<ShockWaveData>& GetShockWaves() { return m_shockWaves; }

		DirectX::SimpleMath::Matrix GetViewMatrix() const
		{
			return m_viewMat;
		}

		DirectX::SimpleMath::Matrix GetProjMatrix() const
		{
			return m_projwMat;
		}

	private:
		f32 m_elapsedTime{ 0.f };
		static PostProcess* s_instance;
		DirectX::SimpleMath::Matrix m_viewMat;
		DirectX::SimpleMath::Matrix m_projwMat;

		// Accessible directly by Effects
		std::vector<LaserBeamData> m_laserBeams;
		std::vector<ShockWaveData> m_shockWaves;

		// Naive ring
		std::vector<DamageDiskData> m_damageDiskDatas;
		u32 m_nextDisk{ 0 };

		f32 m_heartbeatIntensity{ 0.f };
		f32 m_heartbeatFactor{ 0.f };

		f32 m_heartbeatTransitionFactor{ 0.f };
	};
}