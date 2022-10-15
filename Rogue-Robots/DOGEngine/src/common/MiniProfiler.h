#pragma once
namespace DOG
{
	class Application;
}
class MiniProfiler
{
	friend DOG::Application;
private:
	class RollingAvg
	{
	public:
		static constexpr u8 n = 100;
		u64 operator()(u64 input)
		{
			m_sum -= m_previousInputs[m_index];
			m_sum += input;
			m_previousInputs[m_index] = input;
			if (++m_index == n)
				m_index = 0;
			return (m_sum + (n / 2)) / n;
		}
	private:
		u64 m_sum = 0;
		u8 m_index = 0;
		u64 m_previousInputs[n] = {};
	};

	inline static std::vector<std::pair<std::string, u64>> s_times;
	inline static std::unordered_map<std::string, RollingAvg> s_avg;

public:
	MiniProfiler(const std::string& name) : m_name(name)
	{
		m_start = std::chrono::high_resolution_clock::now();
	}

	~MiniProfiler()
	{
		u64 time = static_cast<u64>(duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_start).count());
		s_times.emplace_back(std::move(m_name), s_avg[m_name](time));
	}
	inline static bool s_isActive = true;
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
	std::string m_name;
};