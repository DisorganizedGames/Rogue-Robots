#pragma once
namespace DOG
{
	class MiniProfiler
	{
	public:
		MiniProfiler(const std::string& name);
		~MiniProfiler();
		
		static void Update();
		static void DrawResultWithImGui(bool& open);

		static bool s_isActive;
	private:
		class RollingAvg
		{
		public:
			static constexpr u8 n = 100;
			u64 operator()(u64 input);
		private:
			u64 m_sum = 0;
			u8 m_index = 0;
			u64 m_previousInputs[n] = {};
		};

		static u64 s_frameCounter;
		static std::unordered_map<std::string, u64> s_times;
		static std::unordered_map<std::string, u64> s_accTime;
		static std::unordered_map<std::string, RollingAvg> s_avg;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
		std::string m_name;
	};
}
#define MINIPROFILE DOG::MiniProfiler miniProfiler(__FUNCTION__);
#define MINIPROFILE_NAMED(name) DOG::MiniProfiler miniProfiler(name);