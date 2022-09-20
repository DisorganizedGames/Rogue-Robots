#pragma once

namespace DOG
{
	namespace gfx
	{
		struct MaterialHandle { u64 handle{ 0 }; friend class TypedHandlePool; };

		enum class MaterialTextureType
		{
			Albedo = 0,
			NormalMap,
			MetallicRoughness,
			Emissive,
			COUNT
		};

		struct Mesh { u64 handle{ 0 }; friend class TypedHandlePool; };

		struct MeshContainer
		{
			Mesh mesh;
			u32 numSubmeshes{ 0 };

			u32 managerID{ UINT_MAX };
		};

		struct StaticModel
		{
			MeshContainer mesh;
			std::vector<MaterialHandle> mats;
		};
	}

	enum class VertexAttribute
	{
		Position,
		Normal,
		UV,
		Tangent
	};

	struct SubmeshMetadata
	{
		u32 vertexStart{ 0 };
		u32 vertexCount{ 0 };
		u32 indexStart{ 0 };
		u32 indexCount{ 0 };
	};
}