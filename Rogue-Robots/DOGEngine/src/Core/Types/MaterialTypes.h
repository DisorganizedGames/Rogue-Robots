#pragma once

namespace DOG::gfx
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
}