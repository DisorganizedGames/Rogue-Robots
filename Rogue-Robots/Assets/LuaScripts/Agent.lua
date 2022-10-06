require("VectorMath")

--Managers
local ObjectManager = require("Object")
--local BehaviorManager = require("Behavior")

--Initializers
local pos = Vector3.new(0.0, 0.0, 3.0)
local rot = Vector3.new(0.0, 0.0, 0.0)
local scale = Vector3.new(1.0, 1.0, 1.0)

local agentStats = {
	hp = 100.0,
	maxHP = 100.0,
	speed = 5.0
}

--Agent script
local Agent = ObjectManager:CreateObject()


function Agent:popBehavior()
	self.behaviorStack[#self.behaviorStack] = nil
end

function Agent:pushBehavior(behavior)
	self.behaviorStack[#self.behaviorStack + 1] = behavior
end

function Agent:doBehavior()
	while self.behaviorStack[#self.behaviorStack]:OnUpdate() == false do
		self:popBehavior() --get rid of stale behaviorStack
	end
end


local idle = ObjectManager:CreateObject()
function idle:OnUpdate()
	return true
end

local death = ObjectManager:CreateObject()
function death:OnUpdate()
	if rot.x < 3.1415 then
		rot.x = rot.x + 3.1415 * DeltaTime
		Entity:ModifyComponent(EntityID, "Transform", rot, 2)
	else
		pos.y = pos.y - 1.0 * DeltaTime
		Entity:ModifyComponent(EntityID, "Transform", pos, 1)
	end
	return pos.y > 0.0
end


local default = ObjectManager:CreateObject()
default.target = 1
default.checkpoints = {
	Vector3.new(1.0, 1.3, 1.2),
	Vector3.new(33.0, 8.3, 38.2),
	Vector3.new(26.0, 2.3, 2.2),
	Vector3.new(12.0, 3.3, 27.2),
	Vector3.new(23.0, 2.3, 11.2),
	Vector3.new(18.0, 2.3, 9.2),
}
function default:OnUpdate()
	local dir = self.checkpoints[self.target] - pos;
	local len = Length(dir)
	dir = dir * (1 / len)
	local move = agentStats.speed * DeltaTime
	pos = pos + dir * move
	if (len - move) < 0.05 then
		local prev = self.target
		self.target = self.target % #self.checkpoints + 1
		--print("Switching target: " .. prev .. " --> " .. self.target .. ": ", self.checkpoints[prev], self.checkpoints[self.target])
	end
	--agentStats.hp = agentStats.hp - 15.0 * DeltaTime  --death timer...
	Entity:ModifyComponent(EntityID, "Transform", pos, 1)
	return agentStats.hp > 0.0
end

--In future move to more specific Agent script
function OnStart()
	Agent.model = Asset:LoadModel("Assets/temp_Robot.fbx")
	
	Agent.transform = Entity:AddComponent(EntityID, "AgentStats", agentStats)
	Agent.transform = Entity:AddComponent(EntityID, "Transform", pos, rot, scale)	--Probably move this to C++ Agent()
	Agent.transform = Entity:AddComponent(EntityID, "Model", Agent.model)
	
	Agent.behaviorStack = {idle, death, default}
end

function OnUpdate()
	Agent:doBehavior()
end
