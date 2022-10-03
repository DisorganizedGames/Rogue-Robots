--Managers
local ObjectManager = require("Object")

--Initializers
local pos = {x = 0.0, y = 0.0, z = 3.0}
local rot = {x = 0, y = 0.0, z = 0.0}
local scale = {x = 1.0, y = 1.0, z = 1.0}

local agentStats = {
	hp = 100.0,
	maxHP = 100.0,
	speed = 1.0
}

local dir = 1.0

--Agent script
local Agent = ObjectManager:CreateObject()

function OnStart()
	Agent.model = Asset:LoadModel("Assets/temp_Robot.fbx")
	
	Agent.transform = Entity:AddComponent(EntityID, "AgentStats", agentStats)
	Agent.transform = Entity:AddComponent(EntityID, "Transform", pos, rot, scale)
	Agent.transform = Entity:AddComponent(EntityID, "Model", Agent.model)
end

function OnUpdate()
	pos.x = pos.x + agentStats.speed * DeltaTime * dir;
	if pos.x > 3.0 then
		dir = -1.0
	end
	if pos.x < -3.0 then
		dir = 1.0
	end
	Entity:ModifyComponent(EntityID, "Transform", pos, 1)
end
