#pragma once
#include "../../RHI/RenderResourceHandles.h"


namespace DOG::gfx
{
#define RG_RESOURCE(name) RGResourceID(#name)

	struct RGResourceID
	{
		std::string name = "";

		RGResourceID() = default;
		RGResourceID(const std::string& name) : name(name) {}

		operator std::string() const
		{
			return name;
		}

		bool operator==(const RGResourceID& other) const
		{
			return name == other.name;
		}

	};

	enum class RGResourceType { Buffer, Texture };
	enum class RGResourceVariant { Declared, Imported, Aliased };


	struct RGTextureDesc
	{
		D3D12_RESOURCE_STATES initState{ D3D12_RESOURCE_STATE_COMMON };
	};

	struct RGBufferDesc
	{

	};
}

/*
	We can switch this hashing function later to use some compile time hashed string
*/
namespace std
{
	template <>
	struct hash<DOG::gfx::RGResourceID>
	{
		std::size_t operator()(const DOG::gfx::RGResourceID& k) const
		{
			return hash<string>()(k.name);
			// return k.hashedString
		}
	};
}