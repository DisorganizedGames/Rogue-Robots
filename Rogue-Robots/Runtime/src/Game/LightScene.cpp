#include "LightScene.h"

#include "GameComponent.h"
#include "FakeComputeLightCulling.h"
#include "PrefabInstantiatorFunctions.h"

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

constexpr inline float InvLerp(float a, float b, float t)
{
	return a != b ? (t - a) / (b - a) : a;
}

constexpr inline float Remap(float x, float in_min, float in_max, float out_min, float out_max)
{
	return std::lerp(out_min, out_max, InvLerp(in_min, in_max, x));
}

Vector3 PlaneIntersectPlanes(XMVECTOR p1, XMVECTOR p2, XMVECTOR p3)
{
	XMVECTOR l1, l2;
	DirectX::XMPlaneIntersectPlane(&l1, &l2, p1, p2);
	return DirectX::XMPlaneIntersectLine(p3, l1, l2);
}


struct TiledShadingFrustumVisDebugComponent
{

};

LightScene::LightScene() : Scene(SceneComponent::Type::LightScene)
{
	DOG::ImGuiMenuLayer::RegisterDebugWindow("FrustumIntersectVis", std::bind(&LightScene::TiledShadingDebugMenu, this, std::placeholders::_1), false, std::make_pair(Key::LCtrl, Key::L));
	DOG::ImGuiMenuLayer::RegisterDebugWindow("LightSpawningWindow", std::bind(&LightScene::LightSpawningDebugWindow, this, std::placeholders::_1), true, std::make_pair(Key::LShift, Key::L));
	m_compute.m_lightScene = this;

	m_rgbMats[0].second.emissiveFactor = { 0.6f, 0, 0, 1 };
	m_rgbMats[0].second.albedoFactor = { 0, 0, 0, 1 };
	m_rgbMats[0].first = CustomMaterialManager::Get().AddMaterial(m_rgbMats[0].second);

	m_rgbMats[1].second.emissiveFactor = { 0, 0.6f, 0, 1 };
	m_rgbMats[1].second.albedoFactor = { 0, 0, 0, 1 };
	m_rgbMats[1].first = CustomMaterialManager::Get().AddMaterial(m_rgbMats[1].second);

	m_rgbMats[2].second.emissiveFactor = { 0, 0, 0.6f, 1 };
	m_rgbMats[2].second.albedoFactor = { 0, 0, 0, 1 };
	m_rgbMats[2].first = CustomMaterialManager::Get().AddMaterial(m_rgbMats[2].second);
}

LightScene::~LightScene()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("FrustumIntersectVis");
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("LightSpawningWindow");
}

void LightScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	

}

void LightScene::Update()
{
	
}

DOG::entity LightScene::AddFrustum(DirectX::SimpleMath::Matrix projection, DirectX::SimpleMath::Matrix view)
{
	Matrix m = view * projection;

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
	AddFace(vec, m_rgbMats[0]);

	vec[0] = PlaneIntersectPlanes(nearP, leftP, topP);
	vec[1] = PlaneIntersectPlanes(nearP, rightP, topP);
	vec[2] = PlaneIntersectPlanes(nearP, rightP, botP);
	vec[3] = PlaneIntersectPlanes(nearP, leftP, botP);
	AddFace(vec, m_rgbMats[0]);

	vec[0] = PlaneIntersectPlanes(nearP, rightP, topP);
	vec[1] = PlaneIntersectPlanes(nearP, rightP, botP);
	vec[2] = PlaneIntersectPlanes(farP, rightP, botP);
	vec[3] = PlaneIntersectPlanes(farP, rightP, topP);
	AddFace(vec, m_rgbMats[1]);

	vec[0] = PlaneIntersectPlanes(nearP, leftP, topP);
	vec[1] = PlaneIntersectPlanes(nearP, leftP, botP);
	vec[2] = PlaneIntersectPlanes(farP, leftP, botP);
	vec[3] = PlaneIntersectPlanes(farP, leftP, topP);
	AddFace(vec, m_rgbMats[1]);

	vec[0] = PlaneIntersectPlanes(nearP, leftP, topP);
	vec[1] = PlaneIntersectPlanes(farP, leftP, topP);
	vec[2] = PlaneIntersectPlanes(farP, rightP, topP);
	vec[3] = PlaneIntersectPlanes(nearP, rightP, topP);
	AddFace(vec, m_rgbMats[2]);

	vec[0] = PlaneIntersectPlanes(nearP, leftP, botP);
	vec[1] = PlaneIntersectPlanes(nearP, rightP, botP);
	vec[2] = PlaneIntersectPlanes(farP, rightP, botP);
	vec[3] = PlaneIntersectPlanes(farP, leftP, botP);
	AddFace(vec, m_rgbMats[2]);
	entity e = CreateEntity();
	return e;
}

std::vector<DirectX::SimpleMath::Vector4> LightScene::ExtractPlanes(DirectX::SimpleMath::Matrix projection, DirectX::SimpleMath::Matrix view, int resX, int resY, int tileSize, Vector2i tile)
{
	Matrix m = projection;
	float tileScaleX = resX * (1.0f / (2.0f * tileSize));
	float tileScaleY = resY * (1.0f / (2.0f * tileSize));

	if (tile.x < 0 || tile.y < 0)
	{
		std::cout << "tile can't be less then 0 in any dimension" << std::endl;
		return {};
	}

	if (tile.x >= (float)resX / tileSize || tile.y >= (float)resY / tileSize)
	{
		std::cout << "tile can't be >= res / tileSize" << std::endl;
		return {};
	}

	float tileBiasX = tileScaleX - (float)tile.x;
	float tileBiasY = tileScaleY - (float)tile.y;


	Vector4 col1 = { m._11 * tileScaleX * 2, 0, -1 + tileBiasX * 2, 0 };
	Vector4 col2 = { 0 , -m._22 * tileScaleY * 2, -1 + tileBiasY * 2, 0 };
	Vector4 col4 = { 0, 0, 1, 0 };


	// This will give the frustum in view space
	Vector4 leftP = col4 + col1;
	Vector4 rightP = col4 - col1;
	Vector4 botP = col4 + col2;
	Vector4 topP = col4 - col2;
	Vector4 nearP = { 0, 0, 1, -1 };
	Vector4 farP = { 0, 0, -1.0f, 10 };



	leftP = DirectX::XMPlaneNormalize(leftP);
	rightP = DirectX::XMPlaneNormalize(rightP);
	botP = DirectX::XMPlaneNormalize(botP);
	topP = DirectX::XMPlaneNormalize(topP);
	nearP = DirectX::XMPlaneNormalize(nearP);
	farP = DirectX::XMPlaneNormalize(farP);
	

	// This will give the frustum in world space
	Matrix viewI = view;
	viewI = viewI.Transpose(); // For a plane a matirx should be inverted and then transposed before transforming the plane; we want to transform with the inverse of view => only need to transpose
	leftP = Vector4::Transform(leftP, viewI);
	rightP = Vector4::Transform(rightP, viewI);
	botP = Vector4::Transform(botP, viewI);
	topP = Vector4::Transform(topP, viewI);
	nearP = Vector4::Transform(nearP, viewI);
	farP = Vector4::Transform(farP, viewI);

	return { leftP , rightP, botP, topP, nearP, farP };
}

DOG::entity LightScene::AddFrustum(DirectX::SimpleMath::Vector4 leftPlane, DirectX::SimpleMath::Vector4 rightPlane, DirectX::SimpleMath::Vector4 bottomPlane, DirectX::SimpleMath::Vector4 topPlane, DirectX::SimpleMath::Vector4 nearPlane, DirectX::SimpleMath::Vector4 farPlane)
{
	/*leftPlane = DirectX::XMPlaneNormalize(leftPlane);
	rightPlane = DirectX::XMPlaneNormalize(rightPlane);
	bottomPlane = DirectX::XMPlaneNormalize(bottomPlane);
	topPlane = DirectX::XMPlaneNormalize(topPlane);
	nearPlane = DirectX::XMPlaneNormalize(nearPlane);
	farPlane = DirectX::XMPlaneNormalize(farPlane);*/
	static int matIndex = 0;
	int index = 0;
	std::vector<Vector3> vec;
	vec.resize(4);

	index = matIndex % 3;
	vec[0] = PlaneIntersectPlanes(farPlane, leftPlane, bottomPlane);
	vec[1] = PlaneIntersectPlanes(farPlane, rightPlane, bottomPlane);
	vec[2] = PlaneIntersectPlanes(farPlane, rightPlane, topPlane);
	vec[3] = PlaneIntersectPlanes(farPlane, leftPlane, topPlane);
	entity f = AddFace(vec, m_rgbMats[index]);
	AddComponent<TiledShadingFrustumVisDebugComponent>(f);

	vec[0] = PlaneIntersectPlanes(nearPlane, leftPlane, topPlane);
	vec[1] = PlaneIntersectPlanes(nearPlane, rightPlane, topPlane);
	vec[2] = PlaneIntersectPlanes(nearPlane, rightPlane, bottomPlane);
	vec[3] = PlaneIntersectPlanes(nearPlane, leftPlane, bottomPlane);
	entity n = AddFace(vec, m_rgbMats[index]);
	AddComponent<TiledShadingFrustumVisDebugComponent>(n);

	index = (matIndex + 1) % 3;
	vec[0] = PlaneIntersectPlanes(farPlane, rightPlane, topPlane);
	vec[1] = PlaneIntersectPlanes(farPlane, rightPlane, bottomPlane);
	vec[2] = PlaneIntersectPlanes(nearPlane, rightPlane, bottomPlane);
	vec[3] = PlaneIntersectPlanes(nearPlane, rightPlane, topPlane);
	entity r = AddFace(vec, m_rgbMats[index]);
	AddComponent<TiledShadingFrustumVisDebugComponent>(r);

	vec[0] = PlaneIntersectPlanes(nearPlane, leftPlane, topPlane);
	vec[1] = PlaneIntersectPlanes(nearPlane, leftPlane, bottomPlane);
	vec[2] = PlaneIntersectPlanes(farPlane, leftPlane, bottomPlane);
	vec[3] = PlaneIntersectPlanes(farPlane, leftPlane, topPlane);
	entity l = AddFace(vec, m_rgbMats[index]);
	AddComponent<TiledShadingFrustumVisDebugComponent>(l);

	index = (matIndex + 2) % 3;
	vec[0] = PlaneIntersectPlanes(nearPlane, leftPlane, topPlane);
	vec[1] = PlaneIntersectPlanes(farPlane, leftPlane, topPlane);
	vec[2] = PlaneIntersectPlanes(farPlane, rightPlane, topPlane);
	vec[3] = PlaneIntersectPlanes(nearPlane, rightPlane, topPlane);
	entity t = AddFace(vec, m_rgbMats[index]);
	AddComponent<TiledShadingFrustumVisDebugComponent>(t);

	vec[0] = PlaneIntersectPlanes(nearPlane, leftPlane, bottomPlane);
	vec[1] = PlaneIntersectPlanes(nearPlane, rightPlane, bottomPlane);
	vec[2] = PlaneIntersectPlanes(farPlane, rightPlane, bottomPlane);
	vec[3] = PlaneIntersectPlanes(farPlane, leftPlane, bottomPlane);
	entity b = AddFace(vec, m_rgbMats[index]);
	AddComponent<TiledShadingFrustumVisDebugComponent>(b);

	matIndex++;

	return NULL_ENTITY;
}

DOG::entity LightScene::AddFace(const std::vector<DirectX::SimpleMath::Vector3>& vertexPoints, const std::pair<DOG::MaterialHandle, DOG::MaterialDesc>& mat)
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
	renderComp.materialDesc = mat.second;
	renderComp.material = mat.first;
	return e;
}

DOG::entity LightScene::AddSphere(DirectX::SimpleMath::Vector3 center, float radius, DirectX::SimpleMath::Vector3 color)
{
	auto shapeCreator = ShapeCreator(Shape::sphere, 16, 16, radius);
	auto shape = shapeCreator.GetResult();
	MeshDesc mesh;
	mesh.indices = shape->mesh.indices;
	mesh.submeshData = shape->submeshes;
	for (auto& [attr, vert] : shape->mesh.vertexData)
	{
		mesh.vertexDataPerAttribute[attr] = vert;
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

std::vector<DOG::entity> LightScene::SpawnStaticLights(DirectX::SimpleMath::Vector3 minAABB, DirectX::SimpleMath::Vector3 maxAABB, int xCount, int yCount, int zCount, float strength, float radius)
{
	

	for (int x = 0; x < xCount; x++)
	{
		for (int y = 0; y < yCount; y++)
		{
			for (int z = 0; z < zCount; z++)
			{
				Vector3 pos(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
				pos.x = Remap(pos.x, 0, xCount, minAABB.x, maxAABB.x);
				pos.y = Remap(pos.y, 0, yCount, minAABB.y, maxAABB.y);
				pos.z = Remap(pos.z, 0, zCount, minAABB.z, maxAABB.z);

				AddEntity(CreateStaticPointLight(pos, { 0.5f, 0.5f, 0.8f }, strength, radius, false));
			}
		}
	}



	/*for (int y = 8; y < 9; y += 5)
	{
		for (int x = 5; x < 60; x += 5)
		{
			for (int z = 10; z < 56; z += 5)
			{
				if (maxCount-- > 0)
				{
					Vector3 pos(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
					AddEntity(CreateStaticPointLight(pos, { 0.5f, 0.5f, 0.8f }, strength, true));
				}
			}
		}
	}*/
	return std::vector<DOG::entity>();
}

void LightScene::TiledShadingDebugMenu(bool& open)
{
	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::BeginMenu("TiledShading"))
		{
			if (ImGui::MenuItem("FrustumIntersectVis", "Ctrl+L"))
			{
				open = true;
			}
			ImGui::EndMenu(); // "TiledShading"
		}
		ImGui::EndMenu(); // "View"
	}

	if (open)
	{
		if (ImGui::Begin("FrustumIntersectVis", &open, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			static bool dragWindowOpen = false;
			static bool resultWindowOpen = false;
			if (ImGui::Button("SphereWindow"))
			{
				dragWindowOpen = !dragWindowOpen;
			}
			LightCullingDebugMenu(dragWindowOpen);

			static int res[2] = { 8, 8 };
			ImGui::InputInt2("res", res);
			if (ImGui::Button("Compute"))
			{
				EntityManager::Get().Collect<TiledShadingFrustumVisDebugComponent>().Do([](entity e, TiledShadingFrustumVisDebugComponent&)
					{
						EntityManager::Get().DeferredEntityDestruction(e);
					});

				m_compute.m_data.spheres.clear();
				EntityManager::Get().Collect<TransformComponent, SphereComponent>().Do([&](TransformComponent& transform, SphereComponent& sp)
					{
						FakeCompute::Sphere sphere;
						Vector3 p = transform.GetPosition();
						sphere.center.x = p.x;
						sphere.center.y = p.y;
						sphere.center.z = p.z;
						sphere.center.w = 1;
						sphere.radius = sp.radius;
						m_compute.m_data.spheres.push_back(sphere);
					});

				m_compute.m_data.res.x = (float)res[0];
				m_compute.m_data.res.y = (float)res[1];
				

				u32 tgx = (int)m_compute.m_data.res.x / m_compute.m_groupSizeX + 1 * static_cast<bool>((int)m_compute.m_data.res.x % m_compute.m_groupSizeX);
				u32 tgy = (int)m_compute.m_data.res.y / m_compute.m_groupSizeY + 1 * static_cast<bool>((int)m_compute.m_data.res.y % m_compute.m_groupSizeY);
				m_compute.m_data.groupCountX = tgx;
				m_compute.m_data.groupCountY = tgy;
				m_compute.m_data.groupCountZ = 1;
				m_compute.Dispatch(tgx, tgy, 1);

				m_cullingResultWindowOpen = true;
			}
			CullingResultWindow();

			if (ImGui::Button("Set proj, view and res"))
			{
				f32 aspectRatio = static_cast<f32>(res[0]) / res[1];
				auto p = XMMatrixPerspectiveFovLH(40.f * XM_PI / 180.f, aspectRatio, 1.0f, 10.0f);
				auto& controller = EntityManager::Get().GetComponent<PlayerControllerComponent>(GetPlayer());
				auto& camera = EntityManager::Get().GetComponent<CameraComponent>(controller.cameraEntity);

				m_compute.m_data.proj = p;
				m_compute.m_data.view = camera.viewMatrix;
				m_compute.m_data.res.x = (float)res[0];
				m_compute.m_data.res.y = (float)res[1];
			}

			if (ImGui::Button("Place frustum"))
			{
				AddFrustum(m_compute.m_data.proj, m_compute.m_data.view);
			}
		}
		ImGui::End(); // "FrustumIntersectVis"
	}
}

void LightScene::LightSpawningDebugWindow(bool& open)
{
	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::BeginMenu("TiledShading"))
		{
			if (ImGui::MenuItem("LightSpawningWindow", "Shift+L"))
			{
				open = true;
			}
			ImGui::EndMenu(); // "LightSpawningWindow"
		}
		ImGui::EndMenu(); // "View"
	}
	if (open)
	{
		if (ImGui::Begin("LightSpawningWindow", &open, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			static float minAABB[3] = { 6.0f, 6.5f, 9.0f };
			static float maxAABB[3] = { 60.0f, 33.0f, 56.0f };
			static float strength = 4.0f;
			static float radius = 7.0f;
			ImGui::DragFloat3("minLimit", minAABB, 0.5f);
			ImGui::DragFloat3("maxLimit", maxAABB, 0.5f);
			ImGui::DragFloat("strength", &strength, 0.05f);
			ImGui::DragFloat("radius", &radius, 0.05f);

			static int countXYZ[3] = { 20, 1, 20 };
			ImGui::DragInt3("countXYZ", countXYZ, 1, 0, 50);
			if (countXYZ[0] * countXYZ[1] * countXYZ[2] >= 2048)
			{
				ImGui::Text("to hight light count");
			}

			if (ImGui::Button("apply"))
			{
				AddEntities(SpawnStaticLights(Vector3(minAABB), Vector3(maxAABB), countXYZ[0], countXYZ[1], countXYZ[2], strength, radius));
			}
		}
		ImGui::End();
	}
}

void LightScene::LightCullingDebugMenu(bool& open)
{
	static entity selectedEntity = NULL_ENTITY;
	auto& em = EntityManager::Get();
	if (!em.Exists(selectedEntity)) selectedEntity = NULL_ENTITY;

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

			auto&& sphereToString = [&](entity e) -> std::tuple<std::string, std::string>
			{
				std::string str1 = "entity: " + std::to_string(e);
				Vector3 p = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
				std::string str2 = "";
				str2 += std::format("{:.1f}", p.x) + ", ";
				str2 += std::format("{:.1f}", p.y) + ", ";
				str2 += std::format("{:.1f}", p.z);
				return { str1, str2 };
			};

			if (ImGui::BeginTable("Spheres", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV))
			{
				ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("Pos", ImGuiTableColumnFlags_NoHide);
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
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
	}
}

void LightScene::CullingResultWindow()
{
	if (m_cullingResultWindowOpen)
	{
		if (ImGui::Begin("CullingResult", &m_cullingResultWindowOpen, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			if (ImGui::BeginTable("Results", m_compute.m_data.groupCountX, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV))
			{
				for (int y = 0; y < m_compute.m_data.groupCountY; y++)
				{
					ImGui::TableNextRow();
					for (int x = 0; x < m_compute.m_data.groupCountX; x++)
					{
						ImGui::TableSetColumnIndex(x);
						int index = x + m_compute.m_data.groupCountX * y;
						assert(index < m_compute.m_data.localLightBuffers.size());
						ImGui::Text("%d", m_compute.m_data.localLightBuffers[index].size());
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
	}
}
