#pragma once
#include "../../Graphics/RHI/RenderResourceHandles.h"

namespace DOG
{
	struct LightHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
	struct MaterialHandle { u64 handle{ 0 }; friend class TypedHandlePool; };
	struct Mesh { u64 handle{ 0 }; friend class TypedHandlePool; };


	namespace gfx
	{

		enum class MaterialTextureType
		{
			Albedo = 0,
			NormalMap,
			MetallicRoughness,
			Emissive,
			COUNT
		};


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

	enum class DataUpdateFrequency
	{
		Never,
		Sometimes,
		PerFrame
	};

	struct MeshDesc
	{
		std::unordered_map<VertexAttribute, std::span<u8>> vertexDataPerAttribute;
		std::span<u32> indices;
		std::vector<SubmeshMetadata> submeshData;
	};

	struct MaterialDesc
	{
		std::optional<gfx::TextureView> albedo, normal, metallicRoughness, emissive;

		DirectX::SimpleMath::Vector4 albedoFactor{ 1.f, 1.f, 1.f, 1.f };
		DirectX::SimpleMath::Vector4 emissiveFactor{ 0.f, 0.f, 0.f, 1.f };
		f32 metallicFactor{ 0.f };
		f32 roughnessFactor{ 1.f };
	};

	enum class LightType
	{
		Point, Spot, Area
	};

	// Should be replaced with DataUpdateFrequency later
	enum class LightUpdateFrequency
	{
		Never,		// Static light
		Sometimes,	// Dynamic (assumed less than once per frame on average)
		PerFrame	// Dynamic (once per frame)
	};

	struct PointLightDesc
	{
		DirectX::SimpleMath::Vector3 position{ 0.f, 0.f, 0.f };
		float radius{ 5.0f };
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		float strength{ 0.f };
	};

	struct SpotLightDesc
	{
		DirectX::SimpleMath::Vector3 position{ 0.f, 0.f, 0.f };
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		DirectX::SimpleMath::Vector3 direction{ 0.f, 0.f, 1.f };
		float strength{ 0.f };
		float cutoffAngle{ 15.f };
		u32 id;
	};

	struct AreaLightDesc
	{

	};


}