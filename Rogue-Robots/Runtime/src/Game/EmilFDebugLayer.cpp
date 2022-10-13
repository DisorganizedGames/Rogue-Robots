#include "EmilFDebugLayer.h"

EmilFDebugLayer::EmilFDebugLayer() noexcept
	: Layer("Emil F debug layer"), 
	  m_entityManager{ DOG::EntityManager::Get() },
	  m_cubeEntity{ DOG::NULL_ENTITY },
	  m_sheetEntity{ DOG::NULL_ENTITY }
{
}

void CreateSpotLight(DirectX::SimpleMath::Vector3 position)
{
	//Light source:
	auto flashlightEntity = DOG::EntityManager::Get().CreateEntity();

	auto cone = DOG::AssetManager::Get().LoadModelAsset("Assets/FlashLight1.glb");
	DOG::EntityManager::Get().AddComponent<DOG::ModelComponent>(flashlightEntity, cone);

	auto& tc = DOG::EntityManager::Get().AddComponent<DOG::TransformComponent>(flashlightEntity)
		.SetScale({ 0.3f, 0.3f, 0.3f })
		.SetRotation({ 0.0f, 0.0f, 0.0f })
		.SetPosition(position);

	auto up = tc.worldMatrix.Up();
	up.Normalize();

	auto& c = DOG::EntityManager::Get().AddComponent<DOG::CameraComponent>(flashlightEntity);
	c.isMainCamera = false;
	c.viewMatrix = DirectX::XMMatrixLookAtLH
	(
		{ tc.GetPosition().x, tc.GetPosition().y, tc.GetPosition().z },
		{ tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z },
		{ up.x, up.y, up.z }
	);
	f32 aspectRatio = (f32)DOG::Window::GetWidth() / DOG::Window::GetHeight();
	c.projMatrix = DirectX::XMMatrixPerspectiveFovLH(80.f * DirectX::XM_PI / 180.f, aspectRatio, 800.f, 0.1f);

	auto dd = DOG::SpotLightDesc();
	dd.position = tc.GetPosition();
	dd.color = { 1.f, 1.f, 1.f };
	dd.direction = { tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z };
	dd.strength = 1.f;
	auto lh = DOG::LightManager::Get().AddSpotLight(dd, DOG::LightUpdateFrequency::PerFrame);
	auto& slc = DOG::EntityManager::Get().AddComponent<DOG::SpotLightComponent>(flashlightEntity);

	slc.handle = lh;
	slc.color = dd.color;
	slc.direction = dd.direction;
	slc.strength = dd.strength;
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

	constexpr auto tessFactor = 1;
	auto sheet = assetManager.LoadShapeAsset(DOG::Shape::sheet, tessFactor);
	m_sheetEntity = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<DOG::ModelComponent>(m_sheetEntity, sheet);
	m_entityManager.AddComponent<DOG::TransformComponent>(m_sheetEntity)
		.SetPosition({ 0.0f, 0.0f, 10.0f })
		.SetScale({ 10.0f, 10.0f, 10.0f })
		.SetRotation({-DirectX::XM_PIDIV2, 0.0f, 0.0f});

	CreateSpotLight({ 0.0f, 0.0f, 0.0f });
	CreateSpotLight({ 7.0f, 0.0f, 0.0f });
}

void EmilFDebugLayer::OnDetach()
{

}

void EmilFDebugLayer::OnUpdate()
{
	//...
}

void EmilFDebugLayer::OnRender()
{
	//...
}

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
		if (ImGui::Button("Create spotlight"))
			CreateSpotLight({ posAsFloat[0], posAsFloat[1], posAsFloat[2] });
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
			slc.direction = { tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z };
			slc.direction.Normalize();
			
			slc.dirty = true;
		}
		ImGui::Text("Forward: (%f,%f,%f)", forward.x, forward.y, forward.z);
		ImGui::Text("Focus point: (%f,%f,%f) ", tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z);
		ImGui::Text("Up: (%f,%f,%f)", up.x, up.y, up.z);

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

void EmilFDebugLayer::OnEvent(DOG::IEvent&)
{
	
}
