#include "ImGuiMenuLayer.h"
#include "AssetManager.h"
#include "../ECS/EntityManager.h"
#include "../EventSystem/KeyboardEvents.h"
#include "../Input/Keyboard.h"
#include "ImGUI/imgui.h"

namespace DOG
{
    std::map<std::string, std::pair<std::function<void(bool&)>, bool>> ImGuiMenuLayer::s_debugWindows;

    ImGuiMenuLayer::ImGuiMenuLayer() noexcept : Layer("ImGuiMenu layer")
    {
        RegisterDebugWindow("Demo window", DemoWindow);
        RegisterDebugWindow("Model spawner", ModelSpawner);
    }

    void ImGuiMenuLayer::OnAttach()
    {
    }

    void ImGuiMenuLayer::OnDetach()
    {
    }

    void ImGuiMenuLayer::OnUpdate()
    {
    }

    void ImGuiMenuLayer::OnRender()
    {
        // The menu bar at the top
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File")) ImGui::EndMenu();
            if (ImGui::BeginMenu("View")) ImGui::EndMenu();

            for (auto& [n, w] : s_debugWindows)
            {
                w.first(w.second);
            }
            ImGui::EndMainMenuBar();
        }
    }

    void ImGuiMenuLayer::OnImGuiRender()
    {
    }

    void ImGuiMenuLayer::OnEvent(IEvent& event)
    {
        if (event.GetEventCategory() == EventCategory::KeyboardEventCategory)
        {
            if (event.GetEventType() == EventType::KeyPressedEvent)
            {
                switch (EVENT(KeyPressedEvent).key)
                {
                case DOG::Key::F1:
                    return;
                }
            }

            if (ImGui::GetIO().WantCaptureKeyboard)
            {
                event.StopPropagation();
            }
        }

        if (event.GetEventCategory() == EventCategory::MouseEventCategory)
        {
            if (ImGui::GetIO().WantCaptureMouse)
            {
                event.StopPropagation();
            }
        }
    }

    void ImGuiMenuLayer::RegisterDebugWindow(const std::string& name, std::function<void(bool&)> func)
    {
        s_debugWindows[name] = std::make_pair(func, false);
    }

    void ImGuiMenuLayer::UnRegisterDebugWindow(const std::string& name)
    {
        assert(s_debugWindows.contains(name));
        s_debugWindows.erase(name);
    }

    void ImGuiMenuLayer::ModelSpawner(bool& open)
    {
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Model spawner"))
            {
                open = true;
            }
            ImGui::EndMenu(); // "View"
        }

        if (open)
        {
            ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Model spawner", &open))
            {
                static int id = 0;
                if (ImGui::InputInt("Model ID", &id))
                {
                    std::cout << id << std::endl;
                }
                static float pos[3] = { 0,0,0 };
                ImGui::InputFloat3("Position", pos);
                static float rot[3] = { 0,0,0 };
                ImGui::InputFloat3("Rotation", rot);
                static float scale[3] = { 1,1,1 };
                ImGui::InputFloat3("Scale", scale);

                if (ImGui::Button("Spawn model"))
                {
                    Asset* asset = AssetManager::Get().GetBaseAsset(id);
                    if (asset)
                    {
                        constexpr float toRad = DirectX::XM_PI / 180.0f;
                        entity e = EntityManager::Get().CreateEntity();
                        EntityManager::Get().AddComponent<ModelComponent>(e, id);
                        EntityManager::Get().AddComponent<TransformComponent>(e,
                            DirectX::SimpleMath::Vector3(pos),
                            toRad * DirectX::SimpleMath::Vector3(rot),
                            DirectX::SimpleMath::Vector3(scale));
                    }
                }
            }
            ImGui::End(); // "Model spawner"
        }
    }

    void ImGuiMenuLayer::DemoWindow(bool& open)
    {
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Demo window"))
            {
                open = true;
            }
            ImGui::EndMenu(); // "View"
        }

        if (open)
        {
            ImGui::ShowDemoWindow(&open);
        }
    }
}
