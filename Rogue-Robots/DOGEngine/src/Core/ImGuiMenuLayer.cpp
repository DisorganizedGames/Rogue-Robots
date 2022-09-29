#include "ImGuiMenuLayer.h"
#include "AssetManager.h"
#include "../ECS/EntityManager.h"
#include "../EventSystem/KeyboardEvents.h"
#include "../Input/Keyboard.h"
#include "ImGUI/imgui.h"

namespace DOG
{
    ImGuiMenuLayer::ImGuiMenuLayer() noexcept : Layer("ImGuiMenu layer")
    {

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
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::BeginMenu("Import"))
                {
                    if (ImGui::BeginMenu("Model"))
                    {
                        if (ImGui::MenuItem("Sponza"))
                        {
                            constexpr std::string_view path = "Assets/Sponza_gltf/glTF/Sponza.gltf";
                            if (std::filesystem::exists(path))
                            {
                                u32 id = DOG::AssetManager::Get().LoadModelAsset(path.data());
                                std::cout << "model ID: " << id << ", path: " << path << std::endl;
                            }
                        }
                        if (ImGui::MenuItem("Suzanne"))
                        {
                            constexpr std::string_view path = "Assets/suzanne.glb";
                            if (std::filesystem::exists(path))
                            {
                                u32 id = DOG::AssetManager::Get().LoadModelAsset(path.data());
                                std::cout << "model ID: " << id << ", path: " << path << std::endl;
                            }
                        }
                        ImGui::EndMenu(); // "Model"
                    }

                    if (ImGui::BeginMenu("Texture"))
                    {
                        if (ImGui::MenuItem("Default"))
                        {

                        }
                        ImGui::EndMenu(); // "Texture"
                    }

                    ImGui::EndMenu(); // "Import"
                }

                ImGui::EndMenu(); // "File"
            }
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Demo window"))
                {
                    m_showDemoWindow = true;
                }
                if (ImGui::MenuItem("Empty window"))
                {
                    m_showEmptyWindow = true;
                }
                if (ImGui::MenuItem("Model spawner"))
                {
                    m_showModelSpawnerWindow = true;
                }
                ImGui::EndMenu(); // "View"
            }
            if (ImGui::BeginMenu("Settings"))
            {
                ImGui::EndMenu(); // "Settings"
            }
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::EndMenu(); // "Help"
            }


            ImGui::EndMainMenuBar();



            // Free floating windows, open from "View"

            if (m_showEmptyWindow)
            {
                ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
                if (ImGui::Begin("Empty window", &m_showEmptyWindow))
                {
                }
                ImGui::End(); // "Empty window"
            }

            if (m_showDemoWindow)
            {
                ImGui::ShowDemoWindow(&m_showDemoWindow);
            }

            if (m_showModelSpawnerWindow)
            {
                ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
                if (ImGui::Begin("Model spawner", &m_showModelSpawnerWindow))
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
}
