local MaxAmmo = 10

local bulletModel = 0
local bullets = {}
function OnStart()
	bulletModel = Asset:LoadModel("Assets/red_cube.glb")
	--entity.AddComponent(e, "Transform", {5.0, 5.0, 5.0}, )
end


function OnUpdate()
	if Input:IsLeftPressed() then
		local entity = Entity:CreateEntity()
		table.insert(bullets, entity)
		Entity:AddComponent(entity, "Model", bulletModel)
		Entity:AddComponent(entity, "Transform", {["x"] = 0.0, ["y"] = 0.0, ["z"] = 10.0}, {["x"] = 0.0, ["y"] = 0.0, ["z"] = 0.0}, {["x"] = 1.0, ["y"] = 1.0, ["z"] = 1.0})
	end
	
	for k, v in pairs(bullets) do
		local t = Entity:GetTransformPosData(v)
		Entity:ModifyComponent(v, "Transform", {["x"] = t["x"] + 1.0 * DeltaTime, ["y"] = t["y"], ["z"] = t["z"]}, 1)
	end
	
end