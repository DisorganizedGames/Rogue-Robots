#pragma once

namespace DOG
{
	enum class TimeType : u64
	{
		Seconds = 1'000'000'000,
		Milliseconds = 1'000'000,
		Microseconds = 1'000,
		Nanoseconds = 1,
	};

	class Timer
	{
	private:
		std::chrono::steady_clock::time_point m_startTime;
		std::chrono::steady_clock::time_point m_endTime;
		bool m_started = false;
	public:
		constexpr Timer() = default;
		~Timer() = default;

		void Start()
		{
			m_startTime = std::chrono::steady_clock::now();
			m_started = true;
		}

		u64 Stop()
		{
			if (!m_started)
			{
				return 0;
			}
			
			m_endTime = std::chrono::steady_clock::now();
			m_started = false;
			return std::chrono::duration_cast<std::chrono::nanoseconds>(m_endTime - m_startTime).count();
		}
	};

	class Time
	{
	private:
		static inline Timer s_timer;
		static inline u64 s_deltaTime = 0;
		static inline f64 s_elapsedTime = 0;

	public:
		template<TimeType type = TimeType::Seconds, typename T = f64>
		static T DeltaTime()
		{
			return s_deltaTime / static_cast<T>(type);
		}

		static f64 ElapsedTime()
		{
			return s_elapsedTime;
		}

		static void Start()
		{
			s_timer.Start();
		}

		static void End()
		{
			s_deltaTime = s_timer.Stop();

			s_elapsedTime += DeltaTime();
		}
	};
}