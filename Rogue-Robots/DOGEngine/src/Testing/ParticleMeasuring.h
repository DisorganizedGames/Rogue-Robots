#pragma once
#include "../Core/Time.h"
#include "../common/Utils.h"
#include "Logger.h"
#include "../ECS/EntityManager.h"

namespace DOG
{
	class ParticleMeasuring
	{
	private:
		Timer m_timer;
		f64 m_elapsed;

		u64 m_sampleIndex = 0;
		std::array<f64, 10'000> m_samples;

		std::unique_ptr<Logger> m_logger;
		u64 m_column;

		std::vector<entity> m_entities;

	public:
		ParticleMeasuring() = default;
		void Initialize() noexcept;

		void Tick() noexcept;

	private:
		template<u64 EmitterCount>
		void EmitterTest(f64 dt);


		template<u64 EmitterCount>
		void RandomEmitterTest(f64 dt);

		void ParticleSortTest(f64 dt);
		
		void CreateParticleSystem(entity e, u64 idx);
		void CreateRandomParticleSystem(entity e, u64 idx);
	};

	template<u64 EmitterCount>
	void ParticleMeasuring::EmitterTest(f64 dt)
	{
		static bool inited = false;
		if (!inited)
		{
			inited = true;
			std::cout << "Started EmitterTest<" << EmitterCount << ">\n";
			auto& mgr = EntityManager::Get();
			for (auto& e: m_entities)
			{
				mgr.DeferredEntityDestruction(e);
			}
			m_entities.clear();
			m_entities.resize(EmitterCount);
			std::generate_n(m_entities.begin(), EmitterCount, [&] { return mgr.CreateEntity(); });

			auto idx = 0;
			for (auto& e: m_entities)
			{
				CreateParticleSystem(e, idx++);
			}
		}

		auto sampleIndex = m_sampleIndex%m_samples.size();

		m_samples[sampleIndex] = dt;
		if (sampleIndex == m_samples.size()-1)
		{
			auto avg = std::accumulate(m_samples.begin(), m_samples.end(), 0.0, std::plus{}) / static_cast<f64>(m_samples.size()) / 1'000'000.0;
			m_logger->PushValue(m_column, avg);
		}
	}

	template<u64 EmitterCount>
	void ParticleMeasuring::RandomEmitterTest(f64 dt)
	{
		static bool inited = false;
		if (!inited)
		{
			inited = true;
			std::cout << "Started RandomEmitterTest<" << EmitterCount << ">\n";
			auto& mgr = EntityManager::Get();
			for (auto& e: m_entities)
			{
				mgr.DeferredEntityDestruction(e);
			}
			m_entities.clear();
			m_entities.resize(EmitterCount);
			std::generate_n(m_entities.begin(), EmitterCount, [&] { return mgr.CreateEntity(); });

			auto idx = 0;
			for (auto& e: m_entities)
			{
				CreateRandomParticleSystem(e, idx++);
			}
		}

		auto sampleIndex = m_sampleIndex%m_samples.size();

		m_samples[sampleIndex] = dt;
		if (sampleIndex == m_samples.size()-1)
		{
			auto avg = std::accumulate(m_samples.begin(), m_samples.end(), 0.0, std::plus{}) / static_cast<f64>(m_samples.size()) / 1'000'000.0;
			m_logger->PushValue(m_column, avg);
		}
	}
}

