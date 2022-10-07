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
#if defined _DEBUG
        RegisterDebugWindow("ECS Debug Panel", ECSPanel);
#endif
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

#if defined _DEBUG
    void ImGuiMenuLayer::ECSPanel(bool& open)
    {
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("ECS"))
			{
				open = true;
			}
			ImGui::EndMenu(); // "View"
		}

        if (open)
        {
            auto& mgr = EntityManager::Get();
            auto& pools = mgr.GetAllComponentPools();
            u32 maxEntitiesInAPool = 0u;
            u32 nrOfValidPools = 0u;
            for (auto& [componentID, componentPool] : pools)
            {
                if (componentPool)
                {
                    nrOfValidPools++;
                    if (componentPool->denseArray.size() > maxEntitiesInAPool)
                    {
                        maxEntitiesInAPool = (u32)componentPool->denseArray.size();
                    }
                }
            }

			ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("ECS Debug Panel", &open))
			{
                static bool clickedOnEntities = false;
                static const ComponentPool* selectedPool = nullptr;
                static entity selectedEntity = NULL_ENTITY;
                float widthOfPanel = ImGui::GetWindowContentRegionMax().x;

                if (selectedEntity != NULL_ENTITY && !mgr.Exists(selectedEntity))
                {
                    selectedEntity = NULL_ENTITY;
                }

                ImGui::Text("Entities alive: %d", mgr.GetNrOfEntities());
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    clickedOnEntities = true;
                ImGui::Separator();
                
                ImVec2 actualCursorPosition = ImGui::GetCursorPos();
                ImVec2 savedCursorPosition = ImGui::GetCursorPos();
                ImGui::Text("Unique component types: %d", nrOfValidPools);
				
                ImVec2 outer_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 12);
                static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_::ImGuiTableFlags_PadOuterX;
                if (!selectedPool && !clickedOnEntities && selectedEntity == NULL_ENTITY)
                {
					if (ImGui::BeginTable("table_allComponents", 2, flags, outer_size))
					{
						ImGui::TableSetupScrollFreeze(0, 1);
						ImGui::TableSetupColumn("Component Type", ImGuiTableColumnFlags_NoHide);
                        ImGui::TableSetupColumn("Entities", ImGuiTableColumnFlags_NoHide);
						ImGui::TableHeadersRow();

						for (auto& [componentID, componentPool] : pools)
						{
							if (componentPool)
							{
								ImGui::TableNextRow();
								for (int column = 0; column < 2; column++)
								{
                                    if (!ImGui::TableSetColumnIndex(column) && column > 0)
                                    {
										continue;
                                    }

									auto [entities, poolName] = componentPool->ReportUsage();
									if (column == 0)
									{
										ImGui::Text("%s", std::string(poolName).c_str());
								
										if (ImGui::IsItemClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
										{
											selectedPool = &componentPool;
										}
									}
                                    else
                                    {
										ImGui::Text("%d", entities.size());
                                    }
								}
							}
						}
						ImGui::EndTable();
                        actualCursorPosition = ImGui::GetCursorPos();
					}
                }
                else if (selectedPool && !clickedOnEntities && selectedEntity == NULL_ENTITY)
                {
                    if (ImGui::BeginTable("table_specificComponent", 1, flags, outer_size))
                    {
                        ImGui::TableSetupScrollFreeze(0, 1);
                        auto [entities, poolName] = (*selectedPool)->ReportUsage();
                        ImGui::TableSetupColumn(std::string(poolName).c_str(), ImGuiTableColumnFlags_NoHide);
                        ImGui::TableHeadersRow();
                    
                        for (size_t i{ 0u }; i < entities.size(); i++)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%d", entities[i]);
                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                            {
                                selectedEntity = entities[i];
                            }
                        }

                        ImGui::EndTable();
                        actualCursorPosition = ImGui::GetCursorPos();
                        ImGui::SetCursorPos(ImVec2(savedCursorPosition.x + widthOfPanel - 24, savedCursorPosition.y - 3));
                        ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                        if (ImGui::Button("<"))
                        {
                            selectedPool = nullptr;
                        }
                        ImGui::PopStyleColor();
                    }
                }
                else if (!selectedPool && clickedOnEntities && selectedEntity == NULL_ENTITY)
                {
                    if (ImGui::BeginTable("table_entities", 1, flags, outer_size))
                    {
                        ImGui::TableSetupScrollFreeze(0, 1);
                        ImGui::TableSetupColumn("Entities", ImGuiTableColumnFlags_NoHide);
                        ImGui::TableHeadersRow();

                        auto& entities = mgr.GetAllEntitiesAlive();
                        for (size_t i{ 0u }; i < entities.size(); i++)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%d", entities[i]);
                            if (ImGui::IsItemClicked())
                            {
                                selectedEntity = entities[i];
                            }
                        }
                        ImGui::EndTable();
                        actualCursorPosition = ImGui::GetCursorPos();
                        ImGui::SetCursorPos(ImVec2(savedCursorPosition.x + widthOfPanel - 24, savedCursorPosition.y - 3));
                        ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                        if (ImGui::Button("<"))
                        {
                            clickedOnEntities = false;
                        }
                        ImGui::PopStyleColor();
                    }
                }
                else
                {
					if (ImGui::BeginTable("table_entity", 1, flags, outer_size))
					{
						ImGui::TableSetupScrollFreeze(0, 1);
                        std::string columnName = std::string("Entity #") + std::to_string(selectedEntity);
						ImGui::TableSetupColumn(columnName.c_str(), ImGuiTableColumnFlags_NoHide);
						ImGui::TableHeadersRow();

                        for (auto& [componentID, componentPool] : pools)
                        {
                            if (componentPool)
                            {
                                auto [entities, poolName] = componentPool->ReportUsage();
                                if (std::find(entities.begin(), entities.end(), selectedEntity) != entities.end())
                                {
									ImGui::TableNextRow();
									ImGui::TableSetColumnIndex(0);
									ImGui::Text("%s", std::string(poolName).c_str());
                                }
                            }
                        }
						ImGui::EndTable();
                        actualCursorPosition = ImGui::GetCursorPos();
						ImGui::SetCursorPos(ImVec2(savedCursorPosition.x + widthOfPanel - 24, savedCursorPosition.y - 3));
						ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                        if (ImGui::Button("<"))
                        {
							selectedEntity = NULL_ENTITY;
                        }
						ImGui::PopStyleColor();
					}
                }
                ImGui::SetCursorPos(actualCursorPosition);
				ImGui::Separator();

				//Systems:
                static int selectedSystemIndex = -1;
				auto& systems = mgr.GetAllSystems();
                savedCursorPosition = ImGui::GetCursorPos();
				ImGui::Text("Unique systems: %d", systems.size());
                if (ImGui::BeginTable("table_systems", 2, flags, outer_size))
                {
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableSetupColumn("System execution order", ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoHide);
                    ImGui::TableHeadersRow();

                    for (size_t i{0u}; i < systems.size(); ++i)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);

                        if (selectedSystemIndex == i)
                        {
                            ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                        }

                        std::string_view name = systems[i]->GetName();
						ImGui::Text("%s", std::string(name).c_str());
                        if (ImGui::IsItemClicked() && selectedSystemIndex != (int)i)
                        {
                            selectedSystemIndex = (int)i;
                            ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                        }

                        ImGui::TableSetColumnIndex(1);
						SystemType systemType = systems[i]->GetType();
						if (systemType == SystemType::Standard)
						{
                            ImGui::Text("Standard");
						}
						else
						{
                            ImGui::Text("Critical");
						}
						if (selectedSystemIndex == i)
						{
							ImGui::PopStyleColor(1);
						}
					}
					ImGui::EndTable();
					actualCursorPosition = ImGui::GetCursorPos();
					ImGui::SetCursorPos(ImVec2(savedCursorPosition.x + widthOfPanel - 46, savedCursorPosition.y - 3));
					ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                    if (ImGui::Button("^"))
                    {
                        if (selectedSystemIndex > 0)
                        {
                            std::swap(systems[selectedSystemIndex], systems[selectedSystemIndex - 1]);
                            selectedSystemIndex--;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("v"))
                    {
						if (selectedSystemIndex < systems.size() - 1)
						{
							std::swap(systems[selectedSystemIndex], systems[selectedSystemIndex + 1]);
							selectedSystemIndex++;
						}
                    }
					ImGui::PopStyleColor();
				}
			}
			ImGui::End(); // "ECS Debug Panel"
        }
    }
#endif
}
