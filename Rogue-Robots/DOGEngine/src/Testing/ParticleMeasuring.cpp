#include "ParticleMeasuring.h"

using namespace DOG;

void ParticleMeasuring::Initialize() noexcept
{
	m_logger = std::make_unique<Logger>("ParticleMeasurements.csv", "ParticleMeasurements - Preserve.csv");
	m_column = m_logger->AddColumn("1024k (512/s) (2s lifetime)");
}

void ParticleMeasuring::Tick() noexcept
{
	u64 dt = m_timer.Stop();
	f64 dt_f = static_cast<f64>(dt);
	m_timer.Start();

	m_sampleIndex++;

	u64 test = m_sampleIndex/m_samples.size();
	switch (test)
	{
	case 0:
		break;
	case 1:
		EmitterTest<1>(dt_f);
		break;
	case 2:
		EmitterTest<2>(dt_f);
		break;
	case 3:
		EmitterTest<4>(dt_f);
		break;
	case 4:
		EmitterTest<8>(dt_f);
		break;
	case 5:
		EmitterTest<16>(dt_f);
		break;
	case 6:
		EmitterTest<32>(dt_f);
		break;
	case 7:
		EmitterTest<64>(dt_f);
		break;
	case 8:
		EmitterTest<128>(dt_f);
		break;
	case 9:
		EmitterTest<256>(dt_f);
		break;
	case 10:
		EmitterTest<512>(dt_f);
		break;
	case 11:
		EmitterTest<1024>(dt_f);
		break;
	default:
		if (m_logger)
		{
			m_logger.reset();
			std::cout << "Finished logging tests!\n";
		}
		break;
	}
}

void ParticleMeasuring::CreateParticleSystem(entity e, u64 idx)
{
	auto& mgr = EntityManager::Get();
	mgr.AddComponent<TransformComponent>(e, DirectX::SimpleMath::Vector3((f32)idx, 0, 0));
	auto& comp = mgr.AddComponent<ParticleEmitterComponent>(e);
	comp.spawnRate = 512.f;
	comp.particleSize = 0.01f;
	comp.particleLifetime = 2.f;
}
