#include "ImGuiMenuLayer.h"
#include <vendor\includes\ImGUI\imgui.h>

using namespace DOG;

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
                        u32 id = DOG::AssetManager::Get().LoadModelAsset("Assets/Sponza_gltf/glTF/Sponza.gltf");
                        std::cout << "loaded sponza, sponza got ID: " << id << std::endl;
                        entity e = DOG::EntityManager::Get().CreateEntity();
                        DOG::EntityManager::Get().AddComponent<TransformComponent>(e).SetScale({ 0.05f, 0.05f, 0.05f});
                        DOG::EntityManager::Get().AddComponent<ModelComponent>(e, id);
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
            if (!ImGui::Begin("Empty window", &m_showEmptyWindow))
            {
            }
            ImGui::End(); // "Empty window"
        }

        if (m_showDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
    }
}

void ImGuiMenuLayer::OnImGuiRender()
{
}

void ImGuiMenuLayer::OnEvent(DOG::IEvent& event)
{
    if (event.GetEventCategory() == EventCategory::KeyboardEventCategory)
    {
		if(event.GetEventType() == EventType::KeyPressedEvent)
		{
            switch (EVENT(KeyPressedEvent).key)
            {
            case Key::F1:
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
