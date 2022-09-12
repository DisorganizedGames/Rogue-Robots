#pragma once

namespace DOG::gfx
{
	struct Mesh { u64 handle{ 0 }; friend class TypedHandlePool; };

	enum class VertexAttributeTemp
	{
		Position,
		UV,
		Normal,
		Tangent
	};

	struct SubmeshMetadataTemp
	{
		u32 vertStart{ 0 };
		u32 vertCount{ 0 };
		u32 indexStart{ 0 };
		u32 indexCount{ 0 };
	};

	struct MeshContainerTemp
	{
		Mesh mesh;
		u32 numSubmeshes{ 0 };

		u32 managerID{ UINT_MAX };
	};
}