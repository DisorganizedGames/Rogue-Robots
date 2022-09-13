#pragma once

namespace DOG
{
	namespace gfx
	{
		struct Mesh { u64 handle{ 0 }; friend class TypedHandlePool; };

		struct MeshContainer
		{
			Mesh mesh;
			u32 numSubmeshes{ 0 };

			u32 managerID{ UINT_MAX };
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