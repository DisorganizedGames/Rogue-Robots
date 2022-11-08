#include "LightScene.h"

#include "GameComponent.h"
#include "FakeComputeLightCulling.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

//Vector3 PlaneIntersectPlanes(Vector4 p1, Vector4 p2, Vector4 p3)
//{
//	Vector3 m1 = { p1.x, p2.x, p3.x };
//	Vector3 m2 = { p1.y, p2.y, p3.y };
//	Vector3 m3 = { p1.z, p2.z, p3.z };
//	Vector3 u = m2.Cross(m3);
//	float denom = m1.Dot(u);
//	assert(abs(denom) >= std::numeric_limits<float>::epsilon());
//	Vector3 d = { p1.w, p2.w, p3.w };
//	Vector3 v = m1.Cross(d);
//	float ood = 1.0f / denom;
//	Vector3 p;
//	p.x = ood * d.Dot(u);
//	p.y = ood * m3.Dot(v);
//	p.z = -ood * m2.Dot(v);
//	return p;
//}

Vector3 PlaneIntersectPlanes(XMVECTOR p1, XMVECTOR p2, XMVECTOR p3)
{
	XMVECTOR l1, l2;
	DirectX::XMPlaneIntersectPlane(&l1, &l2, p1, p2);
	return DirectX::XMPlaneIntersectLine(p3, l1, l2);
}


LightScene::LightScene() : Scene(SceneComponent::Type::LightScene)
{
	DOG::ImGuiMenuLayer::RegisterDebugWindow("Tiled Shading", std::bind(&LightScene::TiledShadingDebugMenu, this, std::placeholders::_1), true, std::make_pair(Key::LCtrl, Key::L));
}

LightScene::~LightScene()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("Tiled Shading");
}

void LightScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	f32 aspectRatio = (f32)Window::GetWidth() / Window::GetHeight();
	auto p = XMMatrixPerspectiveFovLH(40.f * XM_PI / 180.f, aspectRatio, 1.0f, 10.0f);
	//AddFrustum(p, DirectX::XMMatrixIdentity());
}

void LightScene::Update()
{
	
}

DOG::entity LightScene::AddFrustum(DirectX::SimpleMath::Matrix projetion, DirectX::SimpleMath::Matrix view)
{
	Matrix m = view * projetion;

	Vector4 leftP = { m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41 };
	Vector4 rightP = { m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41 };
	Vector4 botP = { m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42 };
	Vector4 topP = { m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42 };
	Vector4 nearP = { m._13, m._23, m._33, m._43 };
	Vector4 farP = { m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43 };

	leftP = DirectX::XMPlaneNormalize(leftP);
	rightP = DirectX::XMPlaneNormalize(rightP);
	botP = DirectX::XMPlaneNormalize(botP);
	topP = DirectX::XMPlaneNormalize(topP);
	nearP = DirectX::XMPlaneNormalize(nearP);
	farP = DirectX::XMPlaneNormalize(farP);

	std::vector<Vector3> vec;
	vec.resize(4);

	vec[0] = PlaneIntersectPlanes(farP, leftP, botP);
	vec[1] = PlaneIntersectPlanes(farP, rightP, botP);
	vec[2] = PlaneIntersectPlanes(farP, rightP, topP);
	vec[3] = PlaneIntersectPlanes(farP, leftP, topP);
	AddFace(vec, { 0.5f, 0, 0 });

	vec[0] = PlaneIntersectPlanes(nearP, leftP, topP);
	vec[1] = PlaneIntersectPlanes(nearP, rightP, topP);
	vec[2] = PlaneIntersectPlanes(nearP, rightP, botP);
	vec[3] = PlaneIntersectPlanes(nearP, leftP, botP);
	AddFace(vec, { 0.5f, 0, 0 });

	vec[0] = PlaneIntersectPlanes(nearP, rightP, topP);
	vec[1] = PlaneIntersectPlanes(nearP, rightP, botP);
	vec[2] = PlaneIntersectPlanes(farP, rightP, botP);
	vec[3] = PlaneIntersectPlanes(farP, rightP, topP);
	AddFace(vec, { 0, 0.5f, 0 });

	vec[0] = PlaneIntersectPlanes(nearP, leftP, topP);
	vec[1] = PlaneIntersectPlanes(nearP, leftP, botP);
	vec[2] = PlaneIntersectPlanes(farP, leftP, botP);
	vec[3] = PlaneIntersectPlanes(farP, leftP, topP);
	AddFace(vec, { 0, 0.5f, 0 });

	vec[0] = PlaneIntersectPlanes(nearP, leftP, topP);
	vec[1] = PlaneIntersectPlanes(farP, leftP, topP);
	vec[2] = PlaneIntersectPlanes(farP, rightP, topP);
	vec[3] = PlaneIntersectPlanes(nearP, rightP, topP);
	AddFace(vec, { 0, 0, 0.5f });

	vec[0] = PlaneIntersectPlanes(nearP, leftP, botP);
	vec[1] = PlaneIntersectPlanes(nearP, rightP, botP);
	vec[2] = PlaneIntersectPlanes(farP, rightP, botP);
	vec[3] = PlaneIntersectPlanes(farP, leftP, botP);
	AddFace(vec, { 0, 0, 0.5f });
	entity e = CreateEntity();
	return e;
}

DOG::entity LightScene::AddFrustum(DirectX::SimpleMath::Vector4 leftPlane, DirectX::SimpleMath::Vector4 rightPlane, DirectX::SimpleMath::Vector4 bottomPlane, DirectX::SimpleMath::Vector4 topPlane, DirectX::SimpleMath::Vector4 nearPlane, DirectX::SimpleMath::Vector4 farPlane)
{
	leftPlane = DirectX::XMPlaneNormalize(leftPlane);
	rightPlane = DirectX::XMPlaneNormalize(rightPlane);
	bottomPlane = DirectX::XMPlaneNormalize(bottomPlane);
	topPlane = DirectX::XMPlaneNormalize(topPlane);
	nearPlane = DirectX::XMPlaneNormalize(nearPlane);
	farPlane = DirectX::XMPlaneNormalize(farPlane);

	std::vector<Vector3> vec;
	vec.resize(4);

	vec[0] = PlaneIntersectPlanes(farPlane, leftPlane, bottomPlane);
	vec[1] = PlaneIntersectPlanes(farPlane, rightPlane, bottomPlane);
	vec[2] = PlaneIntersectPlanes(farPlane, rightPlane, topPlane);
	vec[3] = PlaneIntersectPlanes(farPlane, leftPlane, topPlane);
	AddFace(vec, { 0.5f, 0, 0 });

	vec[0] = PlaneIntersectPlanes(nearPlane, leftPlane, topPlane);
	vec[1] = PlaneIntersectPlanes(nearPlane, rightPlane, topPlane);
	vec[2] = PlaneIntersectPlanes(nearPlane, rightPlane, bottomPlane);
	vec[3] = PlaneIntersectPlanes(nearPlane, leftPlane, bottomPlane);
	AddFace(vec, { 0.5f, 0, 0 });

	vec[0] = PlaneIntersectPlanes(nearPlane, rightPlane, topPlane);
	vec[1] = PlaneIntersectPlanes(nearPlane, rightPlane, bottomPlane);
	vec[2] = PlaneIntersectPlanes(farPlane, rightPlane, bottomPlane);
	vec[3] = PlaneIntersectPlanes(farPlane, rightPlane, topPlane);
	AddFace(vec, { 0, 0.5f, 0 });

	vec[0] = PlaneIntersectPlanes(nearPlane, leftPlane, topPlane);
	vec[1] = PlaneIntersectPlanes(nearPlane, leftPlane, bottomPlane);
	vec[2] = PlaneIntersectPlanes(farPlane, leftPlane, bottomPlane);
	vec[3] = PlaneIntersectPlanes(farPlane, leftPlane, topPlane);
	AddFace(vec, { 0, 0.5f, 0 });

	vec[0] = PlaneIntersectPlanes(nearPlane, leftPlane, topPlane);
	vec[1] = PlaneIntersectPlanes(farPlane, leftPlane, topPlane);
	vec[2] = PlaneIntersectPlanes(farPlane, rightPlane, topPlane);
	vec[3] = PlaneIntersectPlanes(nearPlane, rightPlane, topPlane);
	AddFace(vec, { 0, 0, 0.5f });

	vec[0] = PlaneIntersectPlanes(nearPlane, leftPlane, bottomPlane);
	vec[1] = PlaneIntersectPlanes(nearPlane, rightPlane, bottomPlane);
	vec[2] = PlaneIntersectPlanes(farPlane, rightPlane, bottomPlane);
	vec[3] = PlaneIntersectPlanes(farPlane, leftPlane, bottomPlane);
	AddFace(vec, { 0, 0, 0.5f });
	entity e = CreateEntity();
	return e;
}

DOG::entity LightScene::AddFace(const std::vector<DirectX::SimpleMath::Vector3>& vertexPoints, Vector3 color)
{
	auto shapeCreator = ShapeCreator(Shape::quadrilateral, vertexPoints);
	auto shape = shapeCreator.GetResult();
	MeshDesc mesh;
	mesh.indices = shape->mesh.indices;
	mesh.submeshData = shape->submeshes;
	for (auto& [attr, vert] : shape->mesh.vertexData)
	{
		mesh.vertexDataPerAttribute[attr] = vert;
	}



	entity e = CreateEntity();
	AddComponent<TransformComponent>(e);
	auto& renderComp = AddComponent<SubmeshRenderer>(e);
	renderComp.mesh = CustomMeshManager::Get().AddMesh(mesh).first;
	renderComp.materialDesc.emissiveFactor.x = color.x;
	renderComp.materialDesc.emissiveFactor.y = color.y;
	renderComp.materialDesc.emissiveFactor.z = color.z;
	renderComp.materialDesc.albedoFactor = { 0, 0, 0, 1 };
	renderComp.material = CustomMaterialManager::Get().AddMaterial(renderComp.materialDesc);
	return e;
}

DOG::entity LightScene::AddSphere(DirectX::SimpleMath::Vector3 center, float radius, DirectX::SimpleMath::Vector3 color)
{
	std::cout << "sphere, x: " << center.x << ", y: " << center.y << ", z: " << center.z << " color:" << color.x << " " << color.y << " " << color.z << "radius: " << radius << std::endl;
	auto shapeCreator = ShapeCreator(Shape::sphere, 16, 16, radius);
	auto shape = shapeCreator.GetResult();
	MeshDesc mesh;
	mesh.indices = shape->mesh.indices;
	mesh.submeshData = shape->submeshes;
	for (auto& [attr, vert] : shape->mesh.vertexData)
	{
		mesh.vertexDataPerAttribute[attr] = vert;
	}

	std::vector<Vector3> vec;
	vec.resize(mesh.vertexDataPerAttribute[VertexAttribute::Position].size_bytes() / sizeof(Vector3));
	memcpy(vec.data(), mesh.vertexDataPerAttribute[VertexAttribute::Position].data(), mesh.vertexDataPerAttribute[VertexAttribute::Position].size_bytes());
	for (int i = 0; i < 4; i++)
	{
		std::cout << "sphere, x: " << vec[i].x << ", y: " << vec[i].y << ", z: " << vec[i].z << std::endl;
	}


	entity e = CreateEntity();
	AddComponent<TransformComponent>(e, center);
	AddComponent<SphereComponent>(e, radius);
	auto& renderComp = AddComponent<SubmeshRenderer>(e);
	renderComp.mesh = CustomMeshManager::Get().AddMesh(mesh).first;
	renderComp.materialDesc.emissiveFactor.x = color.x;
	renderComp.materialDesc.emissiveFactor.y = color.y;
	renderComp.materialDesc.emissiveFactor.z = color.z;
	renderComp.materialDesc.albedoFactor = { 0, 0, 0, 1 };
	renderComp.material = CustomMaterialManager::Get().AddMaterial(renderComp.materialDesc);
	return e;
}

void LightScene::TiledShadingDebugMenu(bool& open)
{
	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("TiledShading", "Ctrl+L"))
		{
			open = true;
		}
		ImGui::EndMenu(); // "View"
	}

	if (open)
	{
		if (ImGui::Begin("TiledShading", &open, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			static bool dragWindowOpen = false;
			if (ImGui::Button("SphereWindow"))
			{
				dragWindowOpen = !dragWindowOpen;
			}
			LightCullingDebugMenu(dragWindowOpen);


			if (ImGui::Button("Compute"))
			{
				/*auto p = XMMatrixPerspectiveFovLH(40.f * XM_PI / 180.f, 1.0f, 1.0f, 10.0f);
				auto& controller = EntityManager::Get().GetComponent<PlayerControllerComponent>(GetPlayer());
				auto& camera = EntityManager::Get().GetComponent<CameraComponent>(controller.cameraEntity);*/

				FakeCompute::s_data.spheres.clear();
				EntityManager::Get().Collect<TransformComponent, SphereComponent>().Do([](entity e, TransformComponent& transform, SphereComponent& sp)
					{
						FakeCompute::Sphere sphere;
						Vector3 p = transform.GetPosition();
						sphere.center.x = p.x;
						sphere.center.y = p.y;
						sphere.center.z = p.z;
						sphere.center.w = 1;
						sphere.radius = sp.radius;
						sphere.culled = false;
						sphere.e = e;
						FakeCompute::s_data.spheres.push_back(sphere);
					});

				FakeCompute::Dispatch(1, 1, 1);

				for (auto& s : FakeCompute::s_data.spheres)
				{
					EntityManager::Get().GetComponent<SphereComponent>(s.e).culled = s.culled;
					if (s.culled)
					{
						std::cout << "cull true " << s.e << std::endl;
					}
					else
					{
						std::cout << "cull false " << s.e << std::endl;
					}
				}

			}


			if(ImGui::Button("Place face"))
			{
				AddFace({ Vector3(-1, 1, 0), Vector3(1, 1, 0), Vector3(1, -1, 0), Vector3(-1, -1, 0) }, {0.5f, 0, 0});

				auto f = DirectX::BoundingFrustum();
				XMFLOAT3 corners[8];
				f.GetCorners(corners);

				std::vector<Vector3> vec;
				vec.resize(4);
				vec[0] = corners[0];
				vec[1] = corners[1];
				vec[2] = corners[2];
				vec[3] = corners[3];


				AddFace(vec, { 0, 1, 0 });
			}
			if (ImGui::Button("Place frustum"))
			{
				f32 aspectRatio = (f32)Window::GetWidth() / Window::GetHeight();
				auto p = XMMatrixPerspectiveFovLH(40.f * XM_PI / 180.f, aspectRatio, 1.0f, 10.0f);
				auto& controller = EntityManager::Get().GetComponent<PlayerControllerComponent>(GetPlayer());
				auto& camera = EntityManager::Get().GetComponent<CameraComponent>(controller.cameraEntity);

				FakeCompute::s_data.proj = p;
				FakeCompute::s_data.view = camera.viewMatrix;

				AddFrustum(p, camera.viewMatrix);
			}

			

		}
		ImGui::End(); // "TiledShading"
	}
}

void LightScene::LightCullingDebugMenu(bool& open)
{
	static entity selectedEntity = NULL_ENTITY;
	auto& em = EntityManager::Get();

	if (open)
	{
		if (ImGui::Begin("SphereTable", &open, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			if (ImGui::BeginPopup("AddSphereTable"))
			{
				static float posAsFloat[3] = { 20.0f, 10.0f, 20.0f };
				static float radius = 0.2f;
				ImGui::DragFloat3("Position", posAsFloat, 0.1f);
				ImGui::DragFloat("radius", &radius, 0.05f);
				if (ImGui::Button("Create sphere"))
				{
					std::random_device rdev;
					std::mt19937 gen(rdev());
					std::uniform_real_distribution<float> dist(0.05f, 0.7f);

					AddSphere({ posAsFloat[0], posAsFloat[1], posAsFloat[2] }, radius, { dist(gen), dist(gen), dist(gen) });
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			if (selectedEntity != NULL_ENTITY)
				ImGui::Text("move entity: %u", selectedEntity);
			else
				ImGui::Text("no selected entity");

			ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Add Sphere").x - 20);
			if (ImGui::Button("Add Sphere"))
			{
				ImGui::OpenPopup("AddSphereTable");
			}

			if (selectedEntity != NULL_ENTITY)
			{
				auto& tc = em.GetComponent<DOG::TransformComponent>(selectedEntity);
				Vector3 pos = tc.GetPosition();
				float posAsFloat[3] = { pos.x, pos.y, pos.z };
				if (ImGui::DragFloat3("Position", posAsFloat, 0.1f))
					tc.SetPosition({ posAsFloat[0], posAsFloat[1], posAsFloat[2] });
			}
			else
			{
				float posAsFloat[3] = {};
				ImGui::DragFloat3("Position", posAsFloat, 0.1f);
			}

			


			std::vector<entity> spheres;
			em.Collect<SphereComponent, TransformComponent>().Do([&spheres](entity e, SphereComponent&, TransformComponent&) { spheres.push_back(e); });

			auto&& sphereToString = [&](entity e) -> std::tuple<std::string, std::string, std::string>
			{
				auto& s = EntityManager::Get().GetComponent<SphereComponent>(e);
				std::string str1 = "entity: " + std::to_string(e);
				Vector3 p = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
				std::string str2 = "";
				str2 += std::format("{:.1f}", p.x) + ", ";
				str2 += std::format("{:.1f}", p.y) + ", ";
				str2 += std::format("{:.1f}", p.z);
				std::string str3 = "culled: ";
				str3 += (s.culled ? "true" : "false");
				return { str1, str2, str3 };
			};

			if (ImGui::BeginTable("Spheres", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV))
			{
				ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("Pos", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("Culled", ImGuiTableColumnFlags_NoHide);
				ImGui::TableHeadersRow();
				for (int i = 0; i < spheres.size(); i++)
				{
					auto row = sphereToString(spheres[i]);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					if (ImGui::Selectable((std::get<0>(row) + "##" + std::to_string(i)).c_str(), selectedEntity == spheres[i], ImGuiSelectableFlags_SpanAllColumns))
					{
						selectedEntity = spheres[i];
					}
					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::Button("remove"))
						{
							em.DeferredEntityDestruction(spheres[i]);
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}
					ImGui::TableSetColumnIndex(1);
					ImGui::Text(std::get<1>(row).c_str());
					ImGui::TableSetColumnIndex(2);
					ImGui::Text(std::get<2>(row).c_str());
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
	}
}
