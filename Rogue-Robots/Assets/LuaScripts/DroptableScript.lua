
local cubeDropTable = {}

function OnStart()
	cubeDropTable.red 	= {1, Asset:LoadModel("Assets/Models/Temporary_Assets/red_cube.glb")}
	cubeDropTable.green = {1, Asset:LoadModel("Assets/Models/Temporary_Assets/green_cube.glb")}
	cubeDropTable.blue 	= {1, Asset:LoadModel("Assets/Models/Temporary_Assets/blue_cube.glb")}
end

function GetDrop(dropItem)
	if type(dropItem) == "table" then
		return DropGuaranteed(dropItem)
	else
		return dropItem
	end
end

function TotalProb(dropTable) 
	tot = 0
	for k, v in pairs(dropTable) do
		tot = tot + (1/v[1])
	end
	return tot
end

function DropGuaranteed(table)
	acc = 0
	rnd = math.random()
	tot = TotalProb(table)

	for key, value in pairs(table) do
		if rnd < acc + ((1/value[1])/tot) then
			return GetDrop(value[2])
		end
		acc = acc + ((1/value[1])/tot)
	end

	return "Error" -- Should never occur
end

function DropFromEnemy(position)
	local drop = DropGuaranteed(cubeDropTable)
	local dropEntity = Scene:CreateEntity(EntityID)
	Entity:AddComponent(dropEntity, "Model", drop)
	Entity:AddComponent(dropEntity, "Transform", position, {x=0,y=0,z=0}, {x=1,y=1,z=1})
end

EventSystem:Register("EnemyDrop", DropFromEnemy)

