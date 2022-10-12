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
		};

		struct StaticModel
		{
			MeshContainer mesh;
			std::vector<MaterialHandle> mats;
		};
	}


	struct LightHandle { u64 handle{ 0 }; friend class TypedHandlePool; };

	enum class LightType
	{
		Point, Spot, Area
	};

	enum class LightUpdateFrequency
	{
		Never,		// Static light
		Sometimes,	// Dynamic (assumed less than once per frame on average)
		PerFrame	// Dynamic (once per frame)
	};

	struct PointLightDesc
	{
		DirectX::SimpleMath::Vector3 position{ 0.f, 0.f, 0.f };
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		float strength{ 1.f };
	};

	struct SpotLightDesc
	{
		DirectX::SimpleMath::Vector3 position{ 0.f, 0.f, 0.f };
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		DirectX::SimpleMath::Vector3 direction{ 0.f, 0.f, 1.f };
		float strength{ 1.f };
		float cutoffAngle{ 15.f };
	};

	struct AreaLightDesc
	{

	};

	enum class VertexAttribute
	{
		Position,
		Normal,
		UV,
		Tangent,
		BlendData
	};

	struct SubmeshMetadata
	{
		u32 vertexStart{ 0 };
		u32 vertexCount{ 0 };
		u32 indexStart{ 0 };
		u32 indexCount{ 0 };
		u32 blendStart{ 0 };
		u32 blendCount{ 0 };
	};
}