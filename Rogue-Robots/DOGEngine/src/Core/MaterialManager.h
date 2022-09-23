#pragma once
namespace DOG
{
	struct Material
	{
		u32 albedo{0};
		u32 normalMap{0};
		u32 metallicRoughness{0};
		u32 emissive{0};
		DirectX::XMFLOAT4 albedoFactor{ 1, 1, 1, 1 };
		DirectX::XMFLOAT3 emissiveFactor{ 0, 0, 0 };
		f32 metallicFactor{0};
		f32 roughnessFactor{1};

		bool dirty = false;
	};


	class MaterialManager
	{
	public:
		MaterialManager() = default;
		~MaterialManager() = default;
		MaterialManager(const MaterialManager& other) = delete;
		MaterialManager& operator=(const MaterialManager& other) = delete;
		u32 AddMaterial(const Material& newMaterial);
		Material& GetMaterial(u32 materialID);
		const Material& GetMaterial(u32 materialID) const;
	private:
		std::vector<Material> m_materials;
	};
}