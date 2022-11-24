#include "SimpleModelCreator.h"
#include "CustomMaterialManager.h"
#include "CustomMeshManager.h"


using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace DOG
{
	SubmeshRenderer CreateSimpleModel(MaterialDesc material, ImportedMesh mesh, std::optional<Matrix> correctionMatrix) noexcept
	{
		SubmeshMetadata subMeshData;
		subMeshData.indexStart = 0;
		subMeshData.vertexStart = 0;
		assert(!mesh.vertexData[VertexAttribute::Position].empty());
		subMeshData.vertexCount = static_cast<u32>(mesh.vertexData[VertexAttribute::Position].size() / sizeof(Vector3));
		subMeshData.indexCount = static_cast<u32>(mesh.indices.size());

		MeshDesc meshDesc;
		meshDesc.indices = mesh.indices;
		meshDesc.submeshData.push_back(subMeshData);

		if (correctionMatrix)
		{
			XMMATRIX cm = *correctionMatrix;

			for (auto& [attr, vert] : mesh.vertexData)
			{
				std::span<XMFLOAT3> view = std::span<XMFLOAT3>(reinterpret_cast<XMFLOAT3*>(vert.data()), vert.size() / sizeof(XMFLOAT3));
				std::vector<XMFLOAT3> vertVec3(vert.size() / sizeof(XMFLOAT3));
				memcpy(vertVec3.data(), view.data(), view.size_bytes());

				switch (attr)
				{
				case VertexAttribute::Position:
					XMVector3TransformCoordStream(view.data(), sizeof(XMFLOAT3), vertVec3.data(), sizeof(XMFLOAT3), vertVec3.size(), cm);
					break;
				case VertexAttribute::Normal:
					XMVector3TransformNormalStream(view.data(), sizeof(XMFLOAT3), vertVec3.data(), sizeof(XMFLOAT3), vertVec3.size(), cm);
					break;
				case VertexAttribute::Tangent:
					XMVector3TransformNormalStream(view.data(), sizeof(XMFLOAT3), vertVec3.data(), sizeof(XMFLOAT3), vertVec3.size(), cm);
					break;
				}
			}
		}

		for (auto& [attr, vert] : mesh.vertexData)
			meshDesc.vertexDataPerAttribute[attr] = vert;

		SubmeshRenderer simpleModel;
		simpleModel.mesh = CustomMeshManager::Get().AddMesh(meshDesc).first;
		simpleModel.material = CustomMaterialManager::Get().AddMaterial(material);
		simpleModel.materialDesc = material;
		return simpleModel;
	}
}
