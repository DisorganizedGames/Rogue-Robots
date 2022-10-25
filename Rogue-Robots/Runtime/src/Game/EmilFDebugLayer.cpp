#include "EmilFDebugLayer.h"

EmilFDebugLayer::EmilFDebugLayer() noexcept
	: Layer("Emil F debug layer"), 
	  m_entityManager{ DOG::EntityManager::Get() },
	  m_cubeEntity{ DOG::NULL_ENTITY },
	  m_sheetEntity{ DOG::NULL_ENTITY }
{
}

DOG::entity CreateSpotLight(DirectX::SimpleMath::Vector3 position, u32 id)
{
	//Light source:
	DOG::entity flashLightEntity = DOG::NULL_ENTITY;
	flashLightEntity = DOG::EntityManager::Get().CreateEntity();

	auto cone = DOG::AssetManager::Get().LoadModelAsset("Assets/FlashLight1.glb");
	DOG::EntityManager::Get().AddComponent<DOG::ModelComponent>(flashLightEntity, cone);

	auto& tc = DOG::EntityManager::Get().AddComponent<DOG::TransformComponent>(flashLightEntity)
		.SetScale({ 0.3f, 0.3f, 0.3f })
		.SetRotation({ 0.0f, 0.0f, 0.0f })
		.SetPosition(position);

	auto up = tc.worldMatrix.Up();
	up.Normalize();

	auto& c = DOG::EntityManager::Get().AddComponent<DOG::CameraComponent>(flashLightEntity);
	c.isMainCamera = false;
	c.viewMatrix = DirectX::XMMatrixLookAtLH
	(
		{ tc.GetPosition().x, tc.GetPosition().y, tc.GetPosition().z },
		{ tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z },
		{ up.x, up.y, up.z }
	);

	auto dd = DOG::SpotLightDesc();
	dd.position = tc.GetPosition();
	dd.color = { 1.f, 1.f, 1.f };
	//dd.direction = { tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z };
	dd.direction = tc.GetForward();
	dd.strength = 0.6f;
	dd.id = id;
	auto lh = DOG::LightManager::Get().AddSpotLight(dd, DOG::LightUpdateFrequency::PerFrame);
	auto& slc = DOG::EntityManager::Get().AddComponent<DOG::SpotLightComponent>(flashLightEntity);

	slc.handle = lh;
	slc.color = dd.color;
	slc.direction = dd.direction;
	slc.strength = dd.strength;
	slc.id = id;

	float fov = ((slc.cutoffAngle + 0.1f) * 2.0f) * DirectX::XM_PI / 180.f;
	c.projMatrix = DirectX::XMMatrixPerspectiveFovLH(fov, 1, 800.f, 0.1f);

	DOG::EntityManager::Get().AddComponent<DOG::ShadowCasterComponent>(flashLightEntity);

	return flashLightEntity;
}

void EmilFDebugLayer::OnAttach()
{
	//Shadows demo scene:
	auto& assetManager = DOG::AssetManager::Get();
	auto cube = assetManager.LoadModelAsset("Assets/red_cube.glb");
	m_cubeEntity = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<DOG::ModelComponent>(m_cubeEntity, cube);
	m_entityManager.AddComponent<DOG::TransformComponent>(m_cubeEntity)
		.SetPosition({ 0.0f, 0.0f, 5.0f })
		.SetScale({ 1.0f, 1.0f, 1.0f })
		.SetRotation({ 0.0f, 0.0f, 0.0f });
	m_entityManager.AddComponent<DOG::ShadowReceiverComponent>(m_cubeEntity);

	m_cubeEntity2 = m_entityManager.CreateEntity();
	auto cube2 = assetManager.LoadModelAsset("Assets/red_cube.glb");
	m_entityManager.AddComponent<DOG::ModelComponent>(m_cubeEntity2, cube2);
	m_entityManager.AddComponent<DOG::TransformComponent>(m_cubeEntity2)
		.SetPosition({ 0.0f, 3.0f, 5.0f })
		.SetScale({ 1.0f, 1.0f, 1.0f })
		.SetRotation({ 0.0f, 0.0f, 0.0f });
	m_entityManager.AddComponent<DOG::ShadowReceiverComponent>(m_cubeEntity2);

	constexpr auto tessFactor = 1;
	auto sheet = assetManager.LoadShapeAsset(DOG::Shape::sheet, tessFactor);
	m_sheetEntity = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<DOG::ModelComponent>(m_sheetEntity, sheet);
	m_entityManager.AddComponent<DOG::TransformComponent>(m_sheetEntity)
		.SetPosition({ 0.0f, 0.0f, 10.0f })
		.SetScale({ 10.0f, 10.0f, 10.0f })
		.SetRotation({-DirectX::XM_PIDIV2, 0.0f, 0.0f});
	m_entityManager.AddComponent<DOG::ShadowReceiverComponent>(m_sheetEntity);


	m_sheetEntity2 = m_entityManager.CreateEntity();
	auto sheet2 = assetManager.LoadShapeAsset(DOG::Shape::sheet, tessFactor);
	m_entityManager.AddComponent<DOG::ModelComponent>(m_sheetEntity2, sheet2);
	m_entityManager.AddComponent<DOG::TransformComponent>(m_sheetEntity2)
		.SetPosition({ 0.0f, 0.0f, 7.0f })
		.SetScale({ 10.0f, 10.0f, 10.0f })
		.SetRotation({ -2 *DirectX::XM_PI, 0.0f, 0.0f });
	m_entityManager.AddComponent<DOG::ShadowReceiverComponent>(m_sheetEntity2);

	m_FlashLightEntity = CreateSpotLight({ 0.0f, 0.0f, 0.0f }, 0);
	CreateSpotLight({0.0f, 0.0f, -5.0f}, 1);
	//CreateSpotLight({ 4.0f, 0.0f, -5.0f }, 2);
	//CreateSpotLight({ -4.0f, -2.0f, -5.0f }, 3);
}

void EmilFDebugLayer::OnDetach()
{

}

void EmilFDebugLayer::OnUpdate()
{
	m_entityManager.Collect<DOG::ThisPlayer, DOG::TransformComponent>().Do([&](DOG::entity, DOG::ThisPlayer&, DOG::TransformComponent& tc)
		{
			auto& slc = m_entityManager.GetComponent<DOG::SpotLightComponent>(m_FlashLightEntity);
			auto& stc = m_entityManager.GetComponent<DOG::TransformComponent>(m_FlashLightEntity);
			auto& cc = m_entityManager.GetComponent<DOG::CameraComponent>(m_FlashLightEntity);

			stc.worldMatrix = tc.worldMatrix;
			stc.SetPosition(stc.GetPosition() + DirectX::SimpleMath::Vector3(1.0f, 1.0f, 0.0f));
			slc.direction = tc.GetForward();
			slc.dirty = true;

			auto up = tc.worldMatrix.Up();
			up.Normalize();

			cc.viewMatrix = DirectX::XMMatrixLookAtLH
			(
				{ stc.GetPosition().x, stc.GetPosition().y, stc.GetPosition().z },
				{ stc.GetPosition().x + stc.GetForward().x, stc.GetPosition().y + stc.GetForward().y, stc.GetPosition().z + stc.GetForward().z },
				{ up.x, up.y, up.z }
			);
		});

}

void EmilFDebugLayer::OnRender()
{
	//...
}

//float lerp(float v0, float v1, float t) {
//	return v0 + t * (v1 - v0);
//}

void EmilFDebugLayer::OnImGuiRender()
{
	static DOG::entity selectedEntity = DOG::NULL_ENTITY;

	ImGui::Begin("Flashlights");
	ImGui::Text("Flashlight entities:");
	m_entityManager.Collect<DOG::TransformComponent, DOG::CameraComponent, DOG::SpotLightComponent>().Do([](DOG::entity e, DOG::TransformComponent&, DOG::CameraComponent&, DOG::SpotLightComponent&)
		{
			if (!DOG::EntityManager::Get().HasComponent<DOG::ThisPlayer>(e))
			{
				ImGui::Text("%d", e);
				if (ImGui::IsItemClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
				{
					selectedEntity = e;
				}
			}
		});

	{
		ImGui::NewLine(); ImGui::NewLine();
		static float posAsFloat[3];
		ImGui::InputFloat3("Position", posAsFloat);
		//if (ImGui::Button("Create spotlight"))
		//	CreateSpotLight({ posAsFloat[0], posAsFloat[1], posAsFloat[2] });
	}
		ImGui::End();

	if (selectedEntity != DOG::NULL_ENTITY)
	{
		auto& tc = m_entityManager.GetComponent<DOG::TransformComponent>(selectedEntity);
		auto& cc = m_entityManager.GetComponent<DOG::CameraComponent>(selectedEntity);
		auto& slc = m_entityManager.GetComponent<DOG::SpotLightComponent>(selectedEntity);

		auto pos = tc.GetPosition();
		auto forward = tc.GetForward();
		forward.Normalize();
		float posAsFloat[3] = { pos.x, pos.y, pos.z };

		auto up = tc.worldMatrix.Up();
		up.Normalize();

		ImGui::Begin("Flashlight Manager");

		if (ImGui::DragFloat3("Position", posAsFloat, 0.1f))
		{
			tc.SetPosition({ posAsFloat[0], posAsFloat[1], posAsFloat[2] });
			cc.viewMatrix = DirectX::XMMatrixLookAtLH
			(
				{ tc.GetPosition().x, tc.GetPosition().y, tc.GetPosition().z },
				{ tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z },
				{ up.x, up.y, up.z }
			);
			slc.dirty = true;
		}
		static float rotDeg[3] = { DirectX::XMConvertToDegrees(tc.worldMatrix.ToEuler().x), DirectX::XMConvertToDegrees(tc.worldMatrix.ToEuler().y), DirectX::XMConvertToDegrees(tc.worldMatrix.ToEuler().z) };
		static float newRotDeg[3] = { DirectX::XMConvertToDegrees(tc.worldMatrix.ToEuler().x), DirectX::XMConvertToDegrees(tc.worldMatrix.ToEuler().y), DirectX::XMConvertToDegrees(tc.worldMatrix.ToEuler().z) };

		if (ImGui::DragFloat3("Rotation (deg)", newRotDeg, 0.1f))
		{
			float xDelta = newRotDeg[0] - rotDeg[0];
			float yDelta = newRotDeg[1] - rotDeg[1];
			float zDelta = newRotDeg[2] - rotDeg[2];

			tc.RotateL({ DirectX::XMConvertToRadians(xDelta), DirectX::XMConvertToRadians(yDelta), DirectX::XMConvertToRadians(zDelta) });

			rotDeg[0] = newRotDeg[0];
			rotDeg[1] = newRotDeg[1];
			rotDeg[2] = newRotDeg[2];

			cc.viewMatrix = DirectX::XMMatrixLookAtLH
			(
				{ tc.GetPosition().x, tc.GetPosition().y, tc.GetPosition().z },
				{ tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z },
				{ up.x, up.y, up.z }
			);
			slc.direction = tc.GetForward();
			slc.direction.Normalize();
			
			slc.dirty = true;
		}
		ImGui::Text("Forward: (%f,%f,%f)", forward.x, forward.y, forward.z);
		ImGui::Text("Focus point: (%f,%f,%f) ", tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z);
		ImGui::Text("Up: (%f,%f,%f)", up.x, up.y, up.z);

		float cutOff = slc.cutoffAngle;
		if (ImGui::DragFloat("Cut off angle", &cutOff))
		{
			slc.cutoffAngle = cutOff;
			float fov = ((slc.cutoffAngle + 0.1f) * 2.0f)* DirectX::XM_PI / 180.f;
			cc.projMatrix = DirectX::XMMatrixPerspectiveFovLH(fov, 1, 800.f, 0.1f);
			slc.dirty = true;
		}

		static bool checked = false;
		if (ImGui::Checkbox("Light camera", &checked))
		{
			if (checked)
			{
				m_entityManager.Collect<DOG::ThisPlayer, DOG::CameraComponent>().Do([](DOG::ThisPlayer&, DOG::CameraComponent& c)
					{
						c.isMainCamera = false;
					});

				auto& c = m_entityManager.GetComponent<DOG::CameraComponent>(selectedEntity);
				c.isMainCamera = true;
			}
			else
			{
				m_entityManager.Collect<DOG::ThisPlayer, DOG::CameraComponent>().Do([](DOG::ThisPlayer&, DOG::CameraComponent& c)
					{
						c.isMainCamera = true;
					});

				auto& c = m_entityManager.GetComponent<DOG::CameraComponent>(selectedEntity);
				c.isMainCamera = false;
			}
		}
		ImGui::End();
	}
}

void EmilFDebugLayer::OnEvent(DOG::IEvent& event)
{
	switch (event.GetEventType())
	{
	case DOG::EventType::KeyPressedEvent:
	{
		DOG::Key k = EVENT(DOG::KeyPressedEvent).key;
		if (k == DOG::Key::F)
		{
			m_entityManager.Collect<DOG::SpotLightComponent>().Do([](DOG::SpotLightComponent& slc)
				{
					if (slc.strength == 0.6f)
						slc.strength = 0.0f;
					else
						slc.strength = 0.6f;
				});
		}
	}
	}
}
