require("VectorMath")

----------------
--  Managers  --
----------------
local ObjectManager = require("Object")
--local BehaviorManager = require("Behavior")  --currently non-functional (coroutines)




-----------------
--  Behaviors  --
-----------------
local idle = ObjectManager:CreateObject()
idle.name = "idle"
local death = ObjectManager:CreateObject()
death.name = "death"
local chasePlayer = ObjectManager:CreateObject()
chasePlayer.name = "chasePlayer"
local default = ObjectManager:CreateObject()
default.name = "default"


-------------
--  Agent  --
-------------
local Agent = ObjectManager:CreateObject()
Agent.behaviorStack = { idle }
Agent.pos = Vector3.New(25.0, 12.0, 25.0)
Agent.rot = Vector3.New(0.0, 0.0, 0.0)
Agent.stats = {
	hp = 100.0,
	maxHP = 100.0,
	speed = 5.0
}

------------------------
--  Define behaviors  --
------------------------
--	idle  --
function idle:OnUpdate()
	return true
end
--	death  --
function death:OnUpdate()
	if Agent.rot.x < math.pi then
		Agent.rot.x = Agent.rot.x + math.pi * DeltaTime
		Entity:ModifyComponent(EntityID, "Transform", Agent.rot, 2)
	else
		Agent.pos.y = Agent.pos.y - 1.0 * DeltaTime
		Entity:ModifyComponent(EntityID, "Transform", Agent.pos, 1)
	end
	if Agent.pos.y < 0.0 then
		Agent:Init()
	end
	return true
end
--  chase player  --
chasePlayer.target = nil
function chasePlayer:OnUpdate()
	if Agent.stats.hp <= 0.0 then
		print("problem, hp less than 0")
	end
	distances = Host:DistanceToPlayers(Agent.pos)
	if distances[1].dist > 10.0 then
		print("Lost sight of player " .. chasePlayer.target)
		chasePlayer.target = nil
	elseif distances[1].dist < 0.05 then
		--print("Attacking player " .. distances[1].id)
	else
		local dir = distances[1].pos - Agent.pos;
		dir = dir * (1 / distances[1].dist)
		Agent.pos = Agent.pos + dir * Agent.stats.speed * DeltaTime
		Entity:ModifyComponent(EntityID, "Transform", Agent.pos, 1)
		--print("Distance to player " .. distances[1].id .. " is " .. distances[1].dist)
	end
		
		return self.target ~= nil
	end
	--	default  --
	default.target = 1
	default.checkpoints = {
		Vector3.New(10.0, 10.3, 1.2),
		Vector3.New(33.0, 8.3, 38.2),
		Vector3.New(26.0, 2.3, 2.2),
		Vector3.New(12.0, 3.3, 27.2),
		Vector3.New(23.0, 2.3, 11.2),
		Vector3.New(18.0, 2.3, 9.2),
	}
	function default:OnUpdate()
		local dir = self.checkpoints[self.target] - Agent.pos;
		local len = Length(dir)
		dir = dir * (1 / len)
		local move = Agent.stats.speed * DeltaTime
		Agent.pos = Agent.pos + dir * move
		if (len - move) < 0.05 then
			local prev = self.target
			self.target = self.target % #self.checkpoints + 1
			--print("Switching target: " .. prev .. " --> " .. self.target .. ": ", self.checkpoints[prev], self.checkpoints[self.target])
		end
		distances = Host:DistanceToPlayers(Agent.pos)
		if distances[1].dist < 8.0 then
			chasePlayer.target = distances[1].playerID
			-- print("Chasing player " .. chasePlayer.target)
			Agent:pushBehavior(chasePlayer)
		end

	Entity:ModifyComponent(EntityID, "Transform", Agent.pos, 1)

	return true
end

---------------------
--  Agent script   --
---------------------

function Agent:Init()
	print("Init Agent")
	while #self.behaviorStack > 1 do
		self:popBehavior()
	end
	self.pos = Vector3.New(25.0, 12.0, 25.0)
	self.rot = Vector3.New(0.0, 0.0, 0.0)
	self.stats.hp = 100.0
	Entity:ModifyComponent(EntityID, "Transform", self.rot, 2)
	Entity:ModifyComponent(EntityID, "Transform", self.pos, 1)
	self:pushBehavior(default)
end

function Agent:popBehavior()
	print("popping " .. self.behaviorStack[#self.behaviorStack].name)
	self.behaviorStack[#self.behaviorStack] = nil
	self:PrintStack()
end

function Agent:pushBehavior(behavior)
	print("pushing " .. behavior.name)
	self.behaviorStack[#self.behaviorStack + 1] = behavior
	self:PrintStack()
end

function Agent:PrintStack()
	print("Behavior stack:")
	for i, b in ipairs(self.behaviorStack) do
		print("    " .. i .. " " .. b.name)
	end
end

function Agent:doBehavior()
	while self.behaviorStack[#self.behaviorStack]:OnUpdate() == false do
		self:popBehavior() --get rid of inactive behaviors on stack
	end
end

function Agent:Damage(damage)
	self.stats.hp = self.stats.hp - damage
	print("Agent took " .. damage .. ". Current HP: " .. self.stats.hp)
	if self.stats.hp <= 0.0 then
		self:Die()
	end
end

function Agent:Die()
	print("Agent dies")
	while #self.behaviorStack > 1 do
		self:popBehavior()
	end
	self:pushBehavior(death)
end

-- In future move to more specific Agent script --
function OnStart()
	Agent:pushBehavior(idle)
	Agent:Init()

	Agent.model = Asset:LoadModel("Assets/suzanne.glb")
	--Agent.model = Asset:LoadModel("Assets/temp_Robot.fbx")
	Entity:AddComponent(EntityID, "Model", Agent.model)
	Entity:AddComponent(EntityID, "AgentStats", Agent.stats)
	Entity:AddComponent(EntityID, "BoxCollider", Vector3.New(1, 1, 1), true)
	Entity:AddComponent(EntityID, "Rigidbody", true)

end

function OnUpdate()
	Agent.pos = Vector3.FromTable(Entity:GetTransformPosData(EntityID))
	Agent:doBehavior()
end

function OnCollisionEnter(self, e1, e2)
	if (Entity:HasComponent(e1, "Bullet") or Entity:HasComponent(e2, "Bullet")) and Agent.stats.hp > 0.0 then
		Agent:Damage(1000)
	end
end

