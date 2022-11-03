#include "LightScene.h"

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


LightScene::LightScene() : Scene(SceneType::LightScene)
{
	DOG::ImGuiMenuLayer::RegisterDebugWindow("Tiled Shading", std::bind(&LightScene::TiledShadingDebugMenu, this, std::placeholders::_1), false, std::make_pair(Key::LCtrl, Key::L));
}

LightScene::~LightScene()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("Tiled Shading");
}

void LightScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	f32 aspectRatio = (f32)Window::GetWidth() / Window::GetHeight();
	auto p = XMMatrixPerspectiveFovLH(40.f * XM_PI / 180.f, aspectRatio, 10.f, 1.0f);

	//AddFrustumDXTK(p, SimpleMath::Matrix::Identity);
	
	AddFrustum(p, DirectX::XMMatrixIdentity());
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
	AddSphere(vec[0], 0.1f, { 0.8f, 0, 0 });

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
	AddSphere(vec[0], 0.1f, { 0, 0.8f, 0 });

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

//DOG::entity LightScene::AddFrustumDXTK(DirectX::SimpleMath::Matrix projetion, DirectX::SimpleMath::Matrix view)
//{
//	auto f = DirectX::BoundingFrustum(projetion);
//	XMFLOAT3 corners[8];
//	f.GetCorners(corners);
//
//	std::vector<Vector3> vec;
//	vec.resize(4);
//
//	XMVECTOR topP, botP, leftP, rightP, farP, nearP;
//
//	f.GetPlanes(&nearP, &farP, &rightP, &leftP, &topP, &botP);
//
//	vec[0] = PlaneIntersectPlanes(farP, leftP, botP);
//	vec[1] = PlaneIntersectPlanes(farP, rightP, botP);
//	vec[2] = PlaneIntersectPlanes(farP, rightP, topP);
//	vec[3] = PlaneIntersectPlanes(farP, leftP, topP);
//	AddFace(vec, { 0.5f, 0, 0 });
//
//
//	Plane p1 = Plane(Vector3(0, -1, 0), Vector3(0, 1, 0));
//	Plane p2 = Plane(Vector3(-1, 0, 0), Vector3(1, 0, 0));
//	Plane p3 = Plane(Vector3(0, 0, 1), Vector3(0, 0, -1));
//	Vector3 intP = PlaneIntersectPlanes(p1, p2, p3);
//	auto wp1 = DirectX::XMPlaneFromPointNormal(Vector3(0, -1, 0), Vector3(0, 1, 0));
//	auto wp2 = DirectX::XMPlaneFromPointNormal(Vector3(-1, 0, 0), Vector3(1, 0, 0));
//	auto wp3 = DirectX::XMPlaneFromPointNormal(Vector3(0, 0, 1), Vector3(0, 0, -1));
//	Vector3 wintP = PlaneIntersectPlanes(wp1, wp2, wp3);
//
//	vec[0] = PlaneIntersectPlanes(nearP, leftP, topP);
//	vec[1] = PlaneIntersectPlanes(nearP, rightP, topP);
//	vec[2] = PlaneIntersectPlanes(nearP, rightP, botP);
//	vec[3] = PlaneIntersectPlanes(nearP, leftP, botP);
//	AddFace(vec, { 0.5f, 0, 0 });
//
//	vec[0] = PlaneIntersectPlanes(nearP, rightP, topP);
//	vec[1] = PlaneIntersectPlanes(nearP, rightP, botP);
//	vec[2] = PlaneIntersectPlanes(farP, rightP, botP);
//	vec[3] = PlaneIntersectPlanes(farP, rightP, topP);
//	AddFace(vec, { 0, 0.5f, 0 });
//
//	vec[0] = PlaneIntersectPlanes(nearP, leftP, topP);
//	vec[1] = PlaneIntersectPlanes(nearP, leftP, botP);
//	vec[2] = PlaneIntersectPlanes(farP, leftP, botP);
//	vec[3] = PlaneIntersectPlanes(farP, leftP, topP);
//	AddSphere(vec[2], 0.1f, { 0.8f, 0, 0 });
//	AddSphere(vec[0], 0.1f, { 0, 0.8f, 0 });
//	AddFace(vec, { 0, 0.5f, 0 });
//
//	/*vec[0] = corners[4];
//	vec[1] = corners[0];
//	vec[2] = corners[1];
//	vec[3] = corners[5];
//	AddFace(vec, { 0, 0, 0.5f });
//
//	vec[0] = corners[6];
//	vec[1] = corners[2];
//	vec[2] = corners[3];
//	vec[3] = corners[7];
//	AddFace(vec, { 0, 0, 0.5f });*/
//	return NULL_ENTITY;
//}

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
	std::cout << "sphere, x: " << center.x << ", y: " << center.y << ", z: " << center.z << " color:" << color.x << " " << color.y << " " << color.z << std::endl;
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
		if (ImGui::Begin("TiledShading", &open))
		{
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
				auto p = XMMatrixPerspectiveFovLH(40.f * XM_PI / 180.f, aspectRatio, 10.f, 1.0f);
				//AddFrustumDXTK(p, SimpleMath::Matrix::Identity);
				auto& t = EntityManager::Get().GetComponent<TransformComponent>(GetPlayer());
				AddFrustum(p, t.worldMatrix.Invert());
			}

		}
		ImGui::End(); // "TiledShading"
	}
}
