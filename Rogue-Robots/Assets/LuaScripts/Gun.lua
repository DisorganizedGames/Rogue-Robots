local MaxAmmo = 10
function OnStart()
	print("Start")

	--print(Input:IsLeftPressed())
	local model = Asset:LoadModel("Assets/red_cube.glb")
	local entity = Entity:CreateEntity()
	print(model)
	Entity:AddComponent(entity, "Model", model)
	Entity:AddComponent(entity, "Transform", {["x"] = 0.0, ["y"] = 0.0, ["z"] = 0.0}, {["x"] = 0.0, ["y"] = 0.0, ["z"] = 0.0}, {["x"] = 1.0, ["y"] = 1.0, ["z"] = 1.0})
	--entity.AddComponent(e, "Transform", {5.0, 5.0, 5.0}, )
end

function OnUpdate()
	--print("Update")
end