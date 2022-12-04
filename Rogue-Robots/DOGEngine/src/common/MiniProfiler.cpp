#include "MiniProfiler.h"
#include "../Core/Window.h"
#include "ImGUI\imgui.h"

namespace DOG
{
	std::unordered_map<std::string, u64> MiniProfiler::s_times;
	std::unordered_map<std::string, u64> MiniProfiler::s_accTime;
	std::unordered_map<std::string, MiniProfiler::RollingAvg> MiniProfiler::s_avg;
	bool MiniProfiler::s_isActive = true;
	u64 MiniProfiler::s_frameCounter = 0;

	MiniProfiler::MiniProfiler(const std::string& name) : m_name(name)
	{
		m_start = std::chrono::high_resolution_clock::now();
	}

	MiniProfiler::~MiniProfiler()
	{
		u64 time = static_cast<u64>(duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_start).count());
		s_accTime[m_name] += time;
	}


	void MiniProfiler::Update()
	{
		for (auto& [n, t] : s_accTime)
		{
			s_times[n] = s_avg[n](t);
			s_frameCounter++;
		}
		s_accTime.clear();
	}

	void MiniProfiler::DrawResultWithImGui(bool& open)
	{
		static bool allowMove = false;
		static bool lockTopLeft = true;
		static int textOpacity = 180;
		if (ImGui::BeginMenu("MiniProfiler"))
		{
			ImGui::MenuItem("Active", nullptr, &open);
			ImGui::MenuItem("Unlock", nullptr, &allowMove);
			ImGui::MenuItem("LockTopLeft", nullptr, &lockTopLeft);
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.8f);
			ImGui::SliderInt("##1", &textOpacity, 70, 255, "Opacity");
			ImGui::EndMenu(); // "MiniProfiler"
		}


		if (open)
		{
			if (lockTopLeft)
			{
				auto r = Window::GetWindowRect();
				ImVec2 pos;
				pos.x = r.left + 30.0f;
				pos.y = r.top + 50.0f;
				ImGui::SetNextWindowPos(pos);
			}
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			if (ImGui::Begin("MiniProfiler", &open, (allowMove ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground)))
			{
				static std::vector<std::pair<std::string, u64>> results;
				results.clear();
				results.reserve(MiniProfiler::s_times.size());
				for (auto& [n, t] : MiniProfiler::s_times)
					results.emplace_back(n, t);
				std::sort(results.begin(), results.end(), [](auto& a, auto& b) {return a.second > b.second; });

				if (ImGui::BeginTable("data", 2))
				{
					for (auto& [n, t] : results)
					{
						double milisec = 1E-6 * t;
						if (milisec > 16)
							ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, textOpacity));
						else if (milisec > 4)
							ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, textOpacity));
						else if (milisec < 1)
							ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, textOpacity));
						else
							ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, textOpacity));

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("%s", n.c_str());
						
						ImGui::TableSetColumnIndex(1);
						ImGui::Text("%-6.2fms", milisec);

						ImGui::PopStyleColor();
					}
					ImGui::EndTable();
				}

				MiniProfiler::s_times.clear();
			}
			ImGui::End(); // "MiniProfiler"
			ImGui::PopStyleColor();
		}
	}


	u64 MiniProfiler::RollingAvg::operator()(u64 input)
	{
		m_sum -= m_previousInputs[m_index];
		m_sum += input;
		m_previousInputs[m_index] = input;
		if (++m_index == n)
			m_index = 0;
		return (m_sum + (n / 2)) / n;
	}
}
