#include "ShapeCreator.h"
#include <Assimp/Importer.hpp>      // C++ importer interface
#include <DirectXMath.h>


namespace DOG
{
	ShapeCreator::ShapeCreator(const Shape shape, u32 longDiv, u32 latDiv, float radius)
	{
		m_loadedModel = std::make_shared<ImportedModel>();

		switch (shape)
		{
		case Shape::cone:
			MakeCone(longDiv);
			break;
		case Shape::sphere:
			MakeSphere(latDiv, longDiv, radius);
			break;
		case Shape::prism:
			MakePrism(longDiv);
			break;
		case Shape::sheet:
			MakeSheet(latDiv, longDiv);
			break;
		default:
			assert(false);
			break;
		}
	}

	ShapeCreator::ShapeCreator(const Shape shape, const std::vector<DirectX::SimpleMath::Vector3>& vertexPoints)
	{
		m_loadedModel = std::make_shared<ImportedModel>();

		switch (shape)
		{
		case Shape::triangle:
			MakeTriangle(vertexPoints);
			break;
		case Shape::quadrilateral:
			MakeQuadrilateral(vertexPoints);
			break;
		default:
			assert(false);
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
	void ShapeCreator::MakePrism(u32 longDiv)
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

		static constexpr f32 radius = 1.0f;
		static constexpr f32 height = 2.0f;
		static const auto base = XMVectorSet(radius, -radius, 0.0f, 0.0f);
		static const auto offset = XMVectorSet(0.0f, height, 0.0f, 0.0f);
		const f32 longitudeAngle = XM_2PI / longDiv;

		// bottom center
		static constexpr u32 centerBottomIdx = 0;
		static const aiVector3D bottomCenter = { 0.0f, -radius, 0.0f };
		positions.push_back(bottomCenter);
		// top center
		static constexpr u32 centerTopIdx = 1;
		static const aiVector3D topCenter = { 0.0f, radius, 0.0f };
		positions.push_back(topCenter);

		// base vertices
		for (u32 iLong = 0; iLong < longDiv; ++iLong)
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
		// calculate and set side indices
		for (u32 iLong = 0; iLong < longDiv; ++iLong)
		{
			// Indices for next 2 polygons in strip
			const auto i = iLong * 2;
			const auto mod = longDiv * 2;
			indices.push_back(i + 2);
			indices.push_back((i + 2) % mod + 2);
			indices.push_back(i + 3);
			indices.push_back((i + 2) % mod + 2);
			indices.push_back((i + 3) % mod + 2);
			indices.push_back(i + 3);
		}
		// calculate and set base indices
		for (u32 iLong = 0; iLong < longDiv; ++iLong)
		{
			// Indices for next 2 polygons in strip
			const auto i = iLong * 2;
			const auto mod = longDiv * 2;
			indices.push_back(i + 2);
			indices.push_back(centerBottomIdx);
			indices.push_back((i + 2) % mod + 2);
			indices.push_back(centerTopIdx);
			indices.push_back(i + 3);
			indices.push_back((i + 3) % mod + 2);
		}
		// Normal data
		SetNormals(positions, indices, normals);

		// Don't know how to uv map
		for (size_t i = 0; i < positions.size(); ++i)
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

	void ShapeCreator::MakeCone(u32 longDiv)
	{
		using namespace DirectX;
		static constexpr f32 radius = 1.0f;
		static constexpr u32 minLongDiv = 3;
		longDiv = longDiv < minLongDiv ? minLongDiv : longDiv;

		m_loadedModel->mesh.vertexData[VertexAttribute::Position] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::UV] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent] = {};

		std::vector<aiVector3D> positions, normals, tangents;
		std::vector<aiVector2D> uvs;
		std::vector<u32>& indices = m_loadedModel->mesh.indices;
		std::vector<SubmeshMetadata>& submeshes = m_loadedModel->submeshes;

		static const auto base = DirectX::XMVectorSet(radius, -radius, 0.0f, 0.0f);
		const f32 longitudeAngle = XM_2PI / longDiv;

		// base positions
		positions.reserve(longDiv);
		for (u32 iLong = 0; iLong < longDiv; ++iLong)
		{
			const auto pos = XMVector3Transform(base, XMMatrixRotationY(longitudeAngle * iLong));
			positions.push_back({ XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos) });
		}
		
		// The center
		static const aiVector3D centerPos = { 0.0f, 0.0f, 0.0f };
		const u32 iCenter = (u32)(positions.size());
		positions.push_back(centerPos);
		// the tip
		static const aiVector3D tipPos = { 0.0f, radius, 0.0f };
		const u32 iTip = (u32)(positions.size());
		positions.push_back(tipPos);

		// Calculate base indices
		indices.reserve(6 * longDiv);
		for (u32 iLong = 0; iLong < longDiv; ++iLong)
		{
			indices.push_back(iCenter);
			indices.push_back((iLong + 1) % longDiv);
			indices.push_back(iLong);
		}
		// Calculate Cone indices
		for (u32 iLong = 0; iLong < longDiv; ++iLong)
		{
			indices.push_back(iLong);
			indices.push_back((iLong + 1) % longDiv);
			indices.push_back(iTip);
		}

		// Normal data
		SetNormals(positions, indices, normals);

		// Don't know how to uv map
		for (size_t i = 0; i < positions.size(); ++i)
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

	void ShapeCreator::MakeTriangle(const std::vector<DirectX::SimpleMath::Vector3>& vertexPoints)
	{
		using namespace DirectX;
		assert(vertexPoints.size() == 3);
		m_loadedModel->mesh.vertexData[VertexAttribute::Position] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::UV] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent] = {};

		std::vector<aiVector3D> positions, normals, tangents;
		std::vector<aiVector2D> uvs;
		std::vector<u32>& indices = m_loadedModel->mesh.indices;
		std::vector<SubmeshMetadata>& submeshes = m_loadedModel->submeshes;

		aiVector3D pos;
		pos.x = vertexPoints[0].x; pos.y = vertexPoints[0].y; pos.z = vertexPoints[0].z;
		positions.push_back(pos);
		pos.x = vertexPoints[1].x; pos.y = vertexPoints[1].y; pos.z = vertexPoints[1].z;
		positions.push_back(pos);
		pos.x = vertexPoints[2].x; pos.y = vertexPoints[2].y; pos.z = vertexPoints[2].z;
		positions.push_back(pos);

		uvs.emplace_back(0.0f, 0.0f);
		uvs.emplace_back(1.0f, 0.0f);
		uvs.emplace_back(1.0f, 1.0f);

		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);


		// Set Normals
		SetNormals(positions, indices, normals);

		// Set Tangents
		for (size_t i = 0; i < positions.size(); ++i)
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

	void ShapeCreator::MakeQuadrilateral(const std::vector<DirectX::SimpleMath::Vector3>& vertexPoints)
	{
		using namespace DirectX;
		assert(vertexPoints.size() == 4);
		m_loadedModel->mesh.vertexData[VertexAttribute::Position] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::UV] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent] = {};

		std::vector<aiVector3D> positions, normals, tangents;
		std::vector<aiVector2D> uvs;
		std::vector<u32>& indices = m_loadedModel->mesh.indices;
		std::vector<SubmeshMetadata>& submeshes = m_loadedModel->submeshes;

		aiVector3D pos;
		pos.x = vertexPoints[0].x; pos.y = vertexPoints[0].y; pos.z = vertexPoints[0].z;
		positions.push_back(pos);
		pos.x = vertexPoints[1].x; pos.y = vertexPoints[1].y; pos.z = vertexPoints[1].z;
		positions.push_back(pos);
		pos.x = vertexPoints[2].x; pos.y = vertexPoints[2].y; pos.z = vertexPoints[2].z;
		positions.push_back(pos);
		pos.x = vertexPoints[3].x; pos.y = vertexPoints[3].y; pos.z = vertexPoints[3].z;
		positions.push_back(pos);

		uvs.emplace_back(0.0f, 0.0f);
		uvs.emplace_back(1.0f, 0.0f);
		uvs.emplace_back(1.0f, 1.0f);
		uvs.emplace_back(0.0f, 1.0f);

		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(0);
		indices.push_back(2);
		indices.push_back(3);


		// Set Normals
		SetNormals(positions, indices, normals);

		// Set Tangents
		for (size_t i = 0; i < positions.size(); ++i)
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

	void ShapeCreator::MakeSheet(u32 xDiv, u32 zDiv)
	{
		using namespace DirectX;
		static constexpr u32 minXDiv = 1;
		static constexpr u32 minZDiv = 1;
		xDiv = xDiv < minXDiv ? minXDiv : xDiv;
		zDiv = zDiv < minZDiv ? minZDiv : zDiv;

		m_loadedModel->mesh.vertexData[VertexAttribute::Position] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::UV] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent] = {};

		std::vector<aiVector3D> positions, normals, tangents;
		std::vector<aiVector2D> uvs;
		std::vector<u32>& indices = m_loadedModel->mesh.indices;
		std::vector<SubmeshMetadata>& submeshes = m_loadedModel->submeshes;

		static constexpr f32 width = 1.0f;
		static constexpr f32 depth = 1.0f;
		static constexpr f32 xSide = width / 2.0f;
		static constexpr f32 zSide = depth / 2.0f;
		const u32 nVertices_x = xDiv + 1;
		const u32 nVertices_z = zDiv + 1;

		// Set Positions and UVs
		{
			const f32 xDivSize = width / f32(xDiv);
			const f32 zDivSize = depth / f32(zDiv);
			const auto bottomLeft = XMVectorSet(-xSide, 0.0f, -zSide, 0.0f);

			for (u32 z = 0, i = 0; z < nVertices_z; ++z)
			{
				const f32 zPos = f32(z) * zDivSize;
				for (u32 x = 0; x < nVertices_x; ++x, ++i)
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

			const auto xz2i = [nVertices_x](u32 x, u32 y)
			{ return (u32)(x + (y * nVertices_x)); };

			for (u32 y = 0; y < zDiv; ++y)
			{
				for (u32 x = 0; x < xDiv; ++x)
				{
					const u32 indexArray[4] = { xz2i(x,y), xz2i(x + 1,y), xz2i(x,y + 1), xz2i(x + 1, y + 1) };
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
		for (size_t i = 0; i < positions.size(); ++i)
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

	void ShapeCreator::MakeSphere(u32 latDiv, u32 longDiv, float radius)
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

		static const auto base = XMVectorSet(0.0f, 0.0f, radius, 0.0f);
		const f32 lattitudeAngle = XM_PI / latDiv;
		const f32 longitudeAngle = XM_2PI / longDiv;

		for (u32 iLat = 1; iLat < latDiv; ++iLat)
		{
			const auto latBase = XMVector3Transform(base, XMMatrixRotationX(lattitudeAngle * iLat));

			for (u32 iLong = 0; iLong < longDiv; ++iLong)
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
		// Set indices
		for (u32 iLat = 0; iLat < latDiv - 2; ++iLat)
		{
			for (u32 iLong = 0; iLong < longDiv - 1; ++iLong)
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
		// indices for cap fans
		for (u32 iLong = 0; iLong < longDiv - 1; ++iLong)
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