require("VectorMath")

----------------
--  Managers  --
----------------
local ObjectManager = require("Object")
--local BehaviorManager = require("Behavior")  --currently non-functional (coroutines)



---------------------
--  Agent script   --
---------------------
local Agent = ObjectManager:CreateObject()
Agent.pos = Vector3.new(25.0, 12.0, 25.0)
Agent.rot = Vector3.new(0.0, 0.0, 0.0)
Agent.stats = {
	hp = 100.0,
	maxHP = 100.0,
	speed = 5.0
}

function Agent:popBehavior()
	self.behaviorStack[#self.behaviorStack] = nil
end

function Agent:pushBehavior(behavior)
	self.behaviorStack[#self.behaviorStack + 1] = behavior
end

function Agent:doBehavior()
	while self.behaviorStack[#self.behaviorStack]:OnUpdate() == false do
		self:popBehavior() --get rid of inactive behaviors on stack
	end
end


-- In future move to more specific Agent script --
function OnStart()

	-----------------------------
	--  Define some behaviors  --
	-----------------------------
	--	idle  --
	local idle = ObjectManager:CreateObject()
	function idle:OnUpdate()
		return true
	end
	--	death  --
	local death = ObjectManager:CreateObject()
	function death:OnUpdate()
		if Agent.rot.x < 3.1415 then
			Agent.rot.x = Agent.rot.x + math.pi * DeltaTime
			Entity:ModifyComponent(EntityID, "Transform", Agent.rot, 2)
		else
			Agent.pos.y = Agent.pos.y - 1.0 * DeltaTime
			Entity:ModifyComponent(EntityID, "Transform", Agent.pos, 1)
		end
		return Agent.pos.y > 0.0
	end
	--  chase player  --
	local chasePlayer = ObjectManager:CreateObject()
	chasePlayer.target = nil
	function chasePlayer:OnUpdate()
		if Agent.stats.hp <= 0.0 then -- Temporary hack
			Agent:pushBehavior(idle)
			Agent:pushBehavior(death)
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
		return Agent.stats.hp > 0.0
	end


	Agent.behaviorStack = {idle, death, default}	--ultimately keep only common behavior

	Agent.model = Asset:LoadModel("Assets/suzanne.glb")
	--Agent.model = Asset:LoadModel("Assets/temp_Robot.fbx")
	Entity:AddComponent(EntityID, "Model", Agent.model)
	Entity:AddComponent(EntityID, "AgentStats", Agent.stats)
	Entity:AddComponent(EntityID, "BoxCollider", Vector3.new(1, 1, 1), true)
	Entity:AddComponent(EntityID, "Rigidbody", true)

	Agent.behaviorStack = {idle, death, default}
end

function OnUpdate()
	Agent:doBehavior()
end

function OnCollisionEnter(self, e1, e2)
	if (Entity:HasComponent(e1, "Bullet") or Entity:HasComponent(e2, "Bullet")) and Agent.stats.hp > 0.0 then
		Agent.stats.hp = Agent.stats.hp - 1000
	end
end

