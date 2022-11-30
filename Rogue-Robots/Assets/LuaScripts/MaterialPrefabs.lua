MaterialPrefabs = {}
local materials = {}

function MaterialPrefabs:AddMaterial(name, material)
	materials[name] = material
end

function MaterialPrefabs:GetMaterial(name)
	return materials[name]
end

function MaterialPrefabs:OnStart()
	print("Lua Material Prefab Initialization")

	-- Frost Effect
	materials["FrostMaterial"] = Render:CreateMaterial({ 0.188, 0.835, 0.784}, 0.0, 0.0, { 0.0, 0.0, 0.0 })
	materials["FrostExplosionMaterial"] = Render:CreateMaterial({ 0.2, 0.6, 0.8 }, 0.0, 0.0, { 2.0, 6.0, 8.0 })
	materials["BulletMaterial"] = Render:CreateMaterial({ 0.2, 0.6, 0.8 }, 0.0, 0.0, { 2.0, 6.0, 8.0 })
	materials["FireMaterial"] = Render:CreateMaterial({ 1.0, 0.165, 0.016 }, 0.0, 0.0, { 0.0, 0.0, 0.0 })
	materials["FireExplosionMaterial"] = Render:CreateMaterial({ 1.0, 0.1, 0.02 }, 0.0, 0.0, { 10.0, 1.0, 0.2 })

end