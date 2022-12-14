#include "ParticleMeasuring.h"

using namespace DOG;

void ParticleMeasuring::Initialize() noexcept
{
	m_logger = std::make_unique<Logger>("ParticleMeasurements.csv", "ParticleMeasurements - Preserve.csv");
	m_column = m_logger->AddColumn("Time");
}

//void ParticleMeasuring::Tick() noexcept
//{
//	u64 dt = m_timer.Stop();
//	f64 dt_f = static_cast<f64>(dt);
//	m_timer.Start();
//
//	m_sampleIndex++;
//
//	u64 test = m_sampleIndex/m_samples.size();
//	switch (test)
//	{
//	case 0:
//		break;
//	case 1:
//		EmitterTest<1>(dt_f);
//		break;
//	case 2:
//		EmitterTest<2>(dt_f);
//		break;
//	case 3:
//		EmitterTest<4>(dt_f);
//		break;
//	case 4:
//		EmitterTest<8>(dt_f);
//		break;
//	case 5:
//		EmitterTest<16>(dt_f);
//		break;
//	case 6:
//		EmitterTest<32>(dt_f);
//		break;
//	case 7:
//		EmitterTest<64>(dt_f);
//		break;
//	case 8:
//		EmitterTest<128>(dt_f);
//		break;
//	case 9:
//		EmitterTest<256>(dt_f);
//		break;
//	case 10:
//		EmitterTest<512>(dt_f);
//		break;
//	case 11:
//		EmitterTest<1024>(dt_f);
//		break;
//	default:
//		if (m_logger)
//		{
//			m_logger.reset();
//			std::cout << "Finished logging tests!\n";
//		}
//		break;
//	}
//}

//void ParticleMeasuring::Tick() noexcept
//{
//	u64 dt = m_timer.Stop();
//	f64 dt_f = static_cast<f64>(dt);
//	m_timer.Start();
//
//	m_sampleIndex++;
//
//	u64 test = m_sampleIndex/m_samples.size();
//	switch (test)
//	{
//	case 0:
//		break;
//	case 1:
//		ParticleSortTest(dt_f);
//		break;
//	default:
//		if (m_logger)
//		{
//			m_logger.reset();
//			std::cout << "\x1b[32mFinished logging tests!\x1b[0m\n";
//		}
//		break;
//	}
//}

void ParticleMeasuring::ParticleSortTest(f64 dt)
{
	static bool inited = false;
	if (!inited)
	{
		inited = true;
		std::cout << "\x1b[33mStarted ParticleSortTest\x1b[0m\n";
		auto& mgr = EntityManager::Get();
		for (auto e: m_entities)
		{
			mgr.DeferredEntityDestruction(e);
		}
		m_entities.clear();
		m_entities.resize(1);
		m_entities[0] = mgr.CreateEntity();
		CreateParticleSystem(m_entities[0], 0);
	}

	auto sampleIndex = m_sampleIndex%m_samples.size();

	m_samples[sampleIndex] = dt;
	if (sampleIndex == m_samples.size()-1)
	{
		auto avg = std::accumulate(m_samples.begin(), m_samples.end(), 0.0, std::plus{}) / static_cast<f64>(m_samples.size()) / 1'000'000.0;
		m_logger->PushValue(m_column, avg);
	}
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
		RandomEmitterTest<1>(dt_f);
		break;
	case 2:
		RandomEmitterTest<2>(dt_f);
		break;
	case 3:
		RandomEmitterTest<4>(dt_f);
		break;
	case 4:
		RandomEmitterTest<8>(dt_f);
		break;
	case 5:
		RandomEmitterTest<16>(dt_f);
		break;
	case 6:
		RandomEmitterTest<32>(dt_f);
		break;
	case 7:
		RandomEmitterTest<64>(dt_f);
		break;
	case 8:
		RandomEmitterTest<128>(dt_f);
		break;
	case 9:
		RandomEmitterTest<256>(dt_f);
		break;
	case 10:
		RandomEmitterTest<512>(dt_f);
		break;
	case 11:
		RandomEmitterTest<1024>(dt_f);
		break;
	case 12:
		RandomEmitterTest<2048>(dt_f);
		break;
	case 13:
		RandomEmitterTest<4096>(dt_f);
		break;
	case 14:
		RandomEmitterTest<8192>(dt_f);
		break;
	default:
		if (m_logger)
		{
			m_logger.reset();
			std::cout << "\x1b[32mFinished logging tests!\x1b[0m\n";
		}
		break;
	}
}

void ParticleMeasuring::CreateParticleSystem(entity e, u64 idx)
{
	auto& mgr = EntityManager::Get();
	mgr.AddComponent<TransformComponent>(e, DirectX::SimpleMath::Vector3((f32)idx, 0, 0));
	auto& comp = mgr.AddComponent<ParticleEmitterComponent>(e);
	comp.spawnRate = 256.f;
	comp.particleSize = 0.01f;
	comp.particleLifetime = 1.f;
}

void ParticleMeasuring::CreateRandomParticleSystem(entity e, u64 idx)
{
	static std::random_device rd;
	static std::uniform_int_distribution behaviorDist(0, 4);
	static std::uniform_int_distribution spawnDist(0, 2);

	auto& mgr = EntityManager::Get();
	mgr.AddComponent<TransformComponent>(e, DirectX::SimpleMath::Vector3((f32)idx, 0, 0));
	auto& comp = mgr.AddComponent<ParticleEmitterComponent>(e);
	comp.spawnRate = 128.f;
	comp.particleSize = 0.01f;
	comp.particleLifetime = .9f;

	switch (spawnDist(rd))
	{
	case 0:
		mgr.AddComponent<ConeSpawnComponent>(e) = { .speed = 5.f };
		break;
	case 1:
		mgr.AddComponent<CylinderSpawnComponent>(e);
		break;
	case 2:
		mgr.AddComponent<BoxSpawnComponent>(e);
		break;
	}

	switch (behaviorDist(rd))
	{
	case 0:
		mgr.AddComponent<GravityBehaviorComponent>(e);
		break;
	case 1:
		mgr.AddComponent<NoGravityBehaviorComponent>(e) = { };
		break;
	case 2:
		mgr.AddComponent<GravityPointBehaviorComponent>(e) = { };
		break;
	case 3:
		mgr.AddComponent<GravityDirectionBehaviorComponent>(e) = { .direction = {0, 1, 0} };
		break;
	case 4:
		mgr.AddComponent<ConstVelocityBehaviorComponent>(e);
		break;
	}
}
