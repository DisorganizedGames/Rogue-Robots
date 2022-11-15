#pragma once
#include "GraphicsTypes.h"

namespace DOG
{
	enum class Shape
	{
		cone,
		prism,
		sphere,
		sheet,
		triangle,
		quadrilateral,
	};

	struct ImportedMaterial
	{
		std::string materialName;
		std::string albedoPath;
		std::string normalMapPath;
		std::string metallicRoughnessPath;
		std::string emissivePath;
		float albedoFactor[4] = { 1, 1, 1, 1 };
		float metallicFactor = 0;
		float roughnessFactor = 1;
		float emissiveFactor[3] = { 0, 0, 0 };
	};

	struct BlendData
	{
		i32 index = -1;
		f32 weight = 0.0f;
	};

	struct JointNode
	{
		std::string name; // tmp for debug
		DirectX::XMFLOAT4X4 transformation;
		i32 parentIdx = -1;
		i32 jointIdx = -1; // will not be needed
	};

	struct AnimationKey
	{
		std::string name; // tmp for debugging
		f32 time;
		DirectX::XMFLOAT4 value;
	};

	struct ImportedTextureFileMip
	{
		std::vector<u8> data;
		u32 width{ 0 };
		u32 height{ 0 };
		DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
	};

	struct ImportedTextureFile
	{
		std::vector<ImportedTextureFileMip> dataPerMip;
		DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
	};

	struct AnimationData
	{
		std::string name;
		f32 duration;
		f32 ticks;
		f32 ticksPerSec;
		// following can and should be concatenated once we have final model(s)
		std::unordered_map<i32, std::vector<AnimationKey>> scaKeys;
		std::unordered_map<i32, std::vector<AnimationKey>> rotKeys;
		std::unordered_map<i32, std::vector<AnimationKey>> posKeys;
	};

	struct ImportedRig
	{
		std::vector<JointNode> nodes;
		std::vector<DirectX::XMFLOAT4X4> jointOffsets;
		std::vector<AnimationData> animations;
	};

	struct ImportedMesh
	{
		std::unordered_map<VertexAttribute, std::vector<u8>> vertexData;
		std::vector<u32> indices;
	};

	struct ImportedModel
	{
		std::vector<ImportedMaterial> materials;
		std::vector<SubmeshMetadata> submeshes;
		ImportedRig animation;
		ImportedMesh mesh;
	};
}