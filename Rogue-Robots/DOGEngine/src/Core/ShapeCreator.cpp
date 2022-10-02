#include "ShapeCreator.h"
#include <Assimp/Importer.hpp>      // C++ importer interface
#include <DirectXMath.h>


namespace DOG
{
	ShapeCreator::ShapeCreator(const Shape shape, u32 longDiv, u32 latDiv)
	{
		m_loadedModel = std::make_shared<ImportedModel>();

		switch (shape)
		{
		case Shape::cone:
			MakeCone(longDiv < 3 ? 3 : longDiv);
			break;
		case Shape::sphere:
			MakeSphere(latDiv < 3 ? 3 : latDiv, longDiv < 3 ? 3 : longDiv);
			break;
		case Shape::prism:
			MakePrism(longDiv < 3 ? 3 : longDiv);
			break;
		case Shape::sheet:
			MakeSheet(latDiv < 1 ? 1 : latDiv, longDiv < 1 ? 1 : longDiv);
			break;
		default:
			break;
		}
	}

	void SetNormals(const std::vector<aiVector3D>& positions, const std::vector<u32>& indices, std::vector<aiVector3D>& normals)
	{
		using namespace DirectX;

		normals.assign(positions.size(), {});
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			// Primitive vertex positions
			XMVECTOR p0 = XMVectorSet(positions[indices[i + 0]].x, positions[indices[i + 0]].y, positions[indices[i + 0]].z, 1.0f);
			XMVECTOR p1 = XMVectorSet(positions[indices[i + 1]].x, positions[indices[i + 1]].y, positions[indices[i + 1]].z, 1.0f);
			XMVECTOR p2 = XMVectorSet(positions[indices[i + 2]].x, positions[indices[i + 2]].y, positions[indices[i + 2]].z, 1.0f);
			// Calc normal
			const auto n = XMVector3Normalize(XMVector3Cross((p1 - p0), (p2 - p0)));
			// Add normals
			normals[indices[i + 0]] += {XMVectorGetX(n), XMVectorGetY(n), XMVectorGetZ(n)};
			normals[indices[i + 1]] += {XMVectorGetX(n), XMVectorGetY(n), XMVectorGetZ(n)};
			normals[indices[i + 2]] += {XMVectorGetX(n), XMVectorGetY(n), XMVectorGetZ(n)};
		}
		// Normalize
		for (auto& n : normals)
			n /= sqrtf((n.x * n.x + n.y * n.y + n.z * n.z));
	}
	void ShapeCreator::MakePrism(const u32 longDiv)
	{
		using namespace DirectX;

		m_loadedModel->mesh.vertexData[VertexAttribute::Position] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::UV] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent] = {};

		std::vector<aiVector3D> positions, normals, tangents;
		std::vector<aiVector2D> uvs;
		std::vector<u32>& indices = m_loadedModel->mesh.indices;
		std::vector<SubmeshMetadata>& submeshes = m_loadedModel->submeshes;

		static const auto base = XMVectorSet(1.0f, -1.0f, 0.0f, 0.0f);
		static const auto offset = XMVectorSet(0.0f, 2.0f, 0.0f, 0.0f);
		const f32 longitudeAngle = XM_2PI / longDiv;

		// near center
		positions.push_back({0.0f,-1.0f,0.0f});
		static constexpr u32 centerBottomIdx = 0;
		// far center
		positions.push_back({ 0.0f,1.0f,0.0f });
		static constexpr u32 centerTopIdx = 1;

		// base vertices
		for (u32 iLong = 0; iLong < longDiv; iLong++)
		{
			// near base
			{
				const auto pos = XMVector3Transform(base, XMMatrixRotationY(longitudeAngle * iLong));
				positions.push_back({ XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos) });
			}
			// far base
			{
				const auto pos = XMVectorAdd(XMVector3Transform(base, XMMatrixRotationY(longitudeAngle * iLong)), offset);
				positions.push_back({ XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos) });
			}
		}
		// side indices
		for (u32 iLong = 0; iLong < longDiv; iLong++)
		{
			const auto i = iLong * 2;
			const auto mod = longDiv * 2;
			indices.push_back(i + 2);
			indices.push_back((i + 2) % mod + 2);
			indices.push_back(i + 1 + 2);
			indices.push_back((i + 2) % mod + 2);
			indices.push_back((i + 3) % mod + 2);
			indices.push_back(i + 1 + 2);
		}
		// base indices
		for (u32 iLong = 0; iLong < longDiv; iLong++)
		{
			const auto i = iLong * 2;
			const auto mod = longDiv * 2;
			indices.push_back(i + 2);
			indices.push_back(centerBottomIdx);
			indices.push_back((i + 2) % mod + 2);
			indices.push_back(centerTopIdx);
			indices.push_back(i + 1 + 2);
			indices.push_back((i + 3) % mod + 2);
		}
		// Normal data
		SetNormals(positions, indices, normals);

		// Don't know how to uv map
		for (size_t i = 0; i < positions.size(); i++)
		{
			uvs.push_back({ 1.0f, 1.0f });
			tangents.push_back({ 1.0f, 1.0f, 1.0f });
		}

		SubmeshMetadata submesh_md{};
		submesh_md.vertexStart = 0;
		submesh_md.vertexCount = (u32)positions.size();
		submesh_md.indexStart = 0;
		submesh_md.indexCount = (u32)indices.size();
		submeshes.push_back(submesh_md);

		m_loadedModel->mesh.vertexData[VertexAttribute::Position].resize(positions.size() * sizeof(positions[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::UV].resize(uvs.size() * sizeof(uvs[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal].resize(normals.size() * sizeof(normals[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].resize(tangents.size() * sizeof(tangents[0]));

		// Copy to standardized buffers
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Position].data(), positions.data(), positions.size() * sizeof(positions[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::UV].data(), uvs.data(), uvs.size() * sizeof(uvs[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Normal].data(), normals.data(), normals.size() * sizeof(normals[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].data(), tangents.data(), tangents.size() * sizeof(tangents[0]));

		m_loadedModel->materials.push_back({});
	}

	void ShapeCreator::MakeCone(const u32 longDiv)
	{
		using namespace DirectX;

		m_loadedModel->mesh.vertexData[VertexAttribute::Position] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::UV] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent] = {};

		std::vector<aiVector3D> positions, normals, tangents;
		std::vector<aiVector2D> uvs;
		std::vector<u32>& indices = m_loadedModel->mesh.indices;
		std::vector<SubmeshMetadata>& submeshes = m_loadedModel->submeshes;

		const auto base = DirectX::XMVectorSet(1.0f, -1.0f, 0.0f, 0.0f);
		const f32 longitudeAngle = 2.0f * DirectX::XM_PI / longDiv;

		// base positions
		positions.reserve(longDiv);
		for (u32 iLong = 0; iLong < longDiv; iLong++)
		{
			const auto pos = XMVector3Transform(base, XMMatrixRotationY(longitudeAngle * iLong));
			positions.push_back({ XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos) });
		}
		// the center
		positions.push_back({ 0.0f, 0.0f, 0.0f });
		const u32 iCenter = (u32)(positions.size() - 1);
		// the tip 
		positions.push_back({ 0.0f, 1.0f, 0.0f });
		const u32 iTip = (u32)(positions.size() - 1);

		// base indices
		indices.reserve(6 * longDiv);
		for (u32 iLong = 0; iLong < longDiv; iLong++)
		{
			indices.push_back(iCenter);
			indices.push_back((iLong + 1) % longDiv);
			indices.push_back(iLong);
		}
		// Cone indices
		for (u32 iLong = 0; iLong < longDiv; iLong++)
		{
			indices.push_back(iLong);
			indices.push_back((iLong + 1) % longDiv);
			indices.push_back(iTip);
		}

		// Normal data
		SetNormals(positions, indices, normals);

		// Don't know how to uv map
		for (size_t i = 0; i < positions.size(); i++)
		{
			uvs.push_back({ 1.0f, 1.0f });
			tangents.push_back({ 1.0f, 1.0f, 1.0f });
		}

		SubmeshMetadata submesh_md{};
		submesh_md.vertexStart = 0;
		submesh_md.vertexCount = (u32)positions.size();
		submesh_md.indexStart = 0;
		submesh_md.indexCount = (u32)indices.size();
		submeshes.push_back(submesh_md);

		m_loadedModel->mesh.vertexData[VertexAttribute::Position].resize(positions.size() * sizeof(positions[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::UV].resize(uvs.size() * sizeof(uvs[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal].resize(normals.size() * sizeof(normals[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].resize(tangents.size() * sizeof(tangents[0]));

		// Copy to standardized buffers
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Position].data(), positions.data(), positions.size() * sizeof(positions[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::UV].data(), uvs.data(), uvs.size() * sizeof(uvs[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Normal].data(), normals.data(), normals.size() * sizeof(normals[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].data(), tangents.data(), tangents.size() * sizeof(tangents[0]));
		m_loadedModel->materials.push_back({});
	}

	void ShapeCreator::MakeSheet(const u32 xDiv, const u32 zDiv)
	{
		using namespace DirectX;
		m_loadedModel->mesh.vertexData[VertexAttribute::Position] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::UV] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent] = {};

		std::vector<aiVector3D> positions, normals, tangents;
		std::vector<aiVector2D> uvs;
		std::vector<u32>& indices = m_loadedModel->mesh.indices;
		std::vector<SubmeshMetadata>& submeshes = m_loadedModel->submeshes;

		constexpr f32 width = 1.0f;
		constexpr f32 height = 1.0f;
		const u32 nVertices_x = xDiv + 1;
		const u32 nVertices_z = zDiv + 1;

		// Set Positions and UVs
		{
			const f32 xSide = width / 2.0f;
			const f32 zSide = height / 2.0f;
			const f32 xDivSize = width / f32(xDiv);
			const f32 zDivSize = height / f32(zDiv);
			const auto bottomLeft = XMVectorSet(-xSide, 0.0f, -zSide, 0.0f);

			for (u32 z = 0, i = 0; z < nVertices_z; z++)
			{
				const f32 zPos = f32(z) * zDivSize;
				for (u32 x = 0; x < nVertices_x; x++, i++)
				{
					const f32 xPos = f32(x) * xDivSize;
					const auto pos = XMVectorAdd(bottomLeft, XMVectorSet(xPos, 0.0f, zPos, 0.0f));
					positions.push_back({ XMVectorGetX(pos), 0.0f, XMVectorGetZ(pos) });
					uvs.push_back({ xPos, 1.f - zPos });
				}
			}
		}

		// Set Indices
		{
			indices.reserve(xDiv * zDiv * 6);

			const auto vxy2i = [nVertices_x](u32 x, u32 y)
			{ return (u32)(x + (y * nVertices_x)); };

			for (u32 y = 0; y < zDiv; y++)
			{
				for (u32 x = 0; x < xDiv; x++)
				{
					const u32 indexArray[4] = { vxy2i(x,y), vxy2i(x + 1,y), vxy2i(x,y + 1), vxy2i(x + 1, y + 1) };
					indices.push_back(indexArray[0]);
					indices.push_back(indexArray[2]);
					indices.push_back(indexArray[1]);
					indices.push_back(indexArray[1]);
					indices.push_back(indexArray[2]);
					indices.push_back(indexArray[3]);
				}
			}
		}

		// Set Normals
		SetNormals(positions, indices, normals);

		// Set Tangents
		for (size_t i = 0; i < positions.size(); i++)
			tangents.push_back({ 1.0f, 1.0f, 1.0f });

		SubmeshMetadata submesh_md{};
		submesh_md.vertexStart = 0;
		submesh_md.vertexCount = (u32)positions.size();
		submesh_md.indexStart = 0;
		submesh_md.indexCount = (u32)indices.size();
		submeshes.push_back(submesh_md);

		m_loadedModel->mesh.vertexData[VertexAttribute::Position].resize(positions.size() * sizeof(positions[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::UV].resize(uvs.size() * sizeof(uvs[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal].resize(normals.size() * sizeof(normals[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].resize(tangents.size() * sizeof(tangents[0]));

		// Copy to standardized buffers
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Position].data(), positions.data(), positions.size() * sizeof(positions[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::UV].data(), uvs.data(), uvs.size() * sizeof(uvs[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Normal].data(), normals.data(), normals.size() * sizeof(normals[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].data(), tangents.data(), tangents.size() * sizeof(tangents[0]));
		m_loadedModel->materials.push_back({});
	}

	void ShapeCreator::MakeSphere(const u32 latDiv, const u32 longDiv)
	{
		using namespace DirectX;
		static constexpr f32 radius = 1.0f;

		m_loadedModel->mesh.vertexData[VertexAttribute::Position] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::UV] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent] = {};

		std::vector<aiVector3D> positions, normals, tangents;
		std::vector<aiVector2D> uvs;
		std::vector<u32>& indices = m_loadedModel->mesh.indices;
		std::vector<SubmeshMetadata>& submeshes = m_loadedModel->submeshes;

		const auto base = XMVectorSet(0.0f, 0.0f, radius, 0.0f);
		const f32 lattitudeAngle = XM_PI / latDiv;
		const f32 longitudeAngle = XM_2PI / longDiv;

		for (u32 iLat = 1; iLat < latDiv; iLat++)
		{
			const auto latBase = XMVector3Transform(base, XMMatrixRotationX(lattitudeAngle * iLat));

			for (u32 iLong = 0; iLong < longDiv; iLong++)
			{
				const auto pos = XMVector3Transform(latBase, XMMatrixRotationZ(longitudeAngle * iLong));
				positions.push_back({ XMVectorGetX(pos), XMVectorGetY(pos) ,XMVectorGetZ(pos) });
			}
		}

		// add the cap vertices
		const auto northPoleIdx = (u32)positions.size();
		positions.push_back({ XMVectorGetX(base), XMVectorGetY(base) ,XMVectorGetZ(base) });
		const auto southPoleIdx = (u32)positions.size();
		positions.push_back({ XMVectorGetX(XMVectorNegate(base)), XMVectorGetY(XMVectorNegate(base)) ,XMVectorGetZ(XMVectorNegate(base)) });

		const auto calcIdx = [latDiv, longDiv](u32 iLat, u32 iLong)
		{ return (u32)(iLat * longDiv + iLong); };

		for (u32 iLat = 0; iLat < latDiv - 2; iLat++)
		{
			for (u32 iLong = 0; iLong < longDiv - 1; iLong++)
			{
				indices.push_back(calcIdx(iLat, iLong));
				indices.push_back(calcIdx(iLat + 1, iLong));
				indices.push_back(calcIdx(iLat, iLong + 1));
				indices.push_back(calcIdx(iLat, iLong + 1));
				indices.push_back(calcIdx(iLat + 1, iLong));
				indices.push_back(calcIdx(iLat + 1, iLong + 1));
			}
			// wrap band
			indices.push_back(calcIdx(iLat, longDiv - 1));
			indices.push_back(calcIdx(iLat + 1, longDiv - 1));
			indices.push_back(calcIdx(iLat, 0));
			indices.push_back(calcIdx(iLat, 0));
			indices.push_back(calcIdx(iLat + 1, longDiv - 1));
			indices.push_back(calcIdx(iLat + 1, 0));
		}
		// cap fans
		for (u32 iLong = 0; iLong < longDiv - 1; iLong++)
		{
			// north
			indices.push_back(northPoleIdx);
			indices.push_back(calcIdx(0, iLong));
			indices.push_back(calcIdx(0, iLong + 1));
			// south
			indices.push_back(calcIdx(latDiv - 2, iLong + 1));
			indices.push_back(calcIdx(latDiv - 2, iLong));
			indices.push_back(southPoleIdx);
		}
		// wrap triangles
		// north
		indices.push_back(northPoleIdx);
		indices.push_back(calcIdx(0, longDiv - 1));
		indices.push_back(calcIdx(0, 0));
		// south
		indices.push_back(calcIdx(latDiv - 2, 0));
		indices.push_back(calcIdx(latDiv - 2, longDiv - 1));
		indices.push_back(southPoleIdx);

		// Normal data
		SetNormals(positions, indices, normals);
		// Don't know how to uv map
		for (size_t i = 0; i < positions.size(); i++)
		{
			tangents.push_back({ 1.0f, 1.0f, 1.0f });
			uvs.push_back({ 1.0f, 1.0f });
		}

		SubmeshMetadata submesh_md{};
		submesh_md.vertexStart = 0;
		submesh_md.vertexCount = (u32)positions.size();
		submesh_md.indexStart = 0;
		submesh_md.indexCount = (u32)indices.size();
		submeshes.push_back(submesh_md);

		m_loadedModel->mesh.vertexData[VertexAttribute::Position].resize(positions.size() * sizeof(positions[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::UV].resize(uvs.size() * sizeof(uvs[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal].resize(normals.size() * sizeof(normals[0]));
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].resize(tangents.size() * sizeof(tangents[0]));

		// Copy to standardized buffers
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Position].data(), positions.data(), positions.size() * sizeof(positions[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::UV].data(), uvs.data(), uvs.size() * sizeof(uvs[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Normal].data(), normals.data(), normals.size() * sizeof(normals[0]));
		std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].data(), tangents.data(), tangents.size() * sizeof(tangents[0]));
		m_loadedModel->materials.push_back({});
	}
}