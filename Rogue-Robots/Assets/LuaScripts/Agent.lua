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
	speed = 15.0,

	roomId = 0,
}

------------------------
--  Define behaviors  --
------------------------
--	idle  --
function idle:OnUpdate()
	return true
end
--	death  --
death.fall = 9
function death:OnUpdate()
	if Agent.rot.x < math.pi then
		Agent.rot.x = Agent.rot.x + math.pi * DeltaTime
		Entity:ModifyComponent(EntityID, "Transform", Agent.rot, 2)
	else
		Agent.pos.y = Agent.pos.y - self.fall * DeltaTime
		self.fall = self.fall + self.fall * DeltaTime
		Entity:ModifyComponent(EntityID, "Transform", Agent.pos, 1)
	end
	if Agent.pos.y < 0.0 then
		Agent:Init()
		self.fall = 9
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
		--print("Attacking player " .. distances[1].entityID, distances[1].playerID, distances[1].dist, distances[1].pos)
		local playerStats = Entity:GetPlayerStats(distances[1].entityID)
		playerStats.health = playerStats.health - 10.0
		print(playerStats.health)
		Entity:ModifyComponent(distances[1].entityID, "PlayerStats", playerStats)
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
	default.target = 0
	default.cooldown = 0
	default.checkpoints = {
		Vector3.New(30.0, 10.3, 41.2),
		Vector3.New(33.0, 18.3, 50.2),
		Vector3.New(46.0, 20.3, 27.2),
		Vector3.New(52.0, 23.3, 47.2),
		Vector3.New(43.0, 8.3, 31.2),
		Vector3.New(38.0, 22.3, 39.2),
	}
	function default:ChangeDir()
		if self.cooldown <= 0 then
			self.target = self.target % #self.checkpoints + 1
			self.dir = Norm(self.checkpoints[self.target] - Agent.pos)
			self.cooldown = 1
		end
	end
	default:ChangeDir()
	function default:OnUpdate()
		if self.cooldown > 0 then
			self.cooldown = self.cooldown - DeltaTime
		end
		local move = Agent.stats.speed * DeltaTime
		Agent.pos = Agent.pos + self.dir * move
		--local dir = Norm(Vector3.New(35.0, 15.0, 40.0) - Agent.pos)
		--Agent.pos = Agent.pos + dir * move
		distances = Host:DistanceToPlayers(Agent.pos)
		if distances[1].dist < 8.0 then
			chasePlayer.target = distances[1].playerID
			-- print("Chasing player " .. chasePlayer.target)
			Agent:pushBehavior(chasePlayer)
		end
	if Length(Vector3.New(35, 15, 40) - Agent.pos) > 100 then
		print("Agent has escaped: " .. Agent.pos)
		Agent:Die()
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
	self.pos = self.spawnPoints[self.nextSpawnPoint]
	self.nextSpawnPoint = self.nextSpawnPoint % #self.spawnPoints + 1
	print("Agent spawned at " .. self.pos)
	self.rot = Vector3.New(0.0, 0.0, 0.0)
	self.stats.hp = 100.0
	Entity:ModifyComponent(EntityID, "Transform", self.rot, 2)
	Entity:ModifyComponent(EntityID, "Transform", self.pos, 1)
	self:pushBehavior(default)
end

Agent.spawnPoints = {
		Vector3.New(13.0, 10.3, 41.2),
		Vector3.New(33.0, 28.3, 5.2),
		Vector3.New(36.0, 27.3, 37.2),
		Vector3.New(42.0, 17.3, 17.2),
		Vector3.New(23.0, 18.3, 31.2),
		Vector3.New(38.0, 12.3, 9.2),
}
Agent.nextSpawnPoint = 1

function Agent:popBehavior()
	--print("popping " .. self.behaviorStack[#self.behaviorStack].name)
	self.behaviorStack[#self.behaviorStack] = nil
	--self:PrintStack()
end

function Agent:pushBehavior(behavior)
	--print("pushing " .. behavior.name)
	self.behaviorStack[#self.behaviorStack + 1] = behavior
	--self:PrintStack()
end

function Agent:PrintStack()
	--print("Behavior stack:")
	--for i, b in ipairs(self.behaviorStack) do
	--	print("    " .. i .. " " .. b.name)
	--end
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
	while #self.behaviorStack > 1 do
		self:popBehavior()
	end
	self:pushBehavior(death)
end

function Agent:Collision(entity)
	b = self.behaviorStack[#self.behaviorStack]
	if b.name == "default" then
		b:ChangeDir()
	end
end

-- In future move to more specific Agent script --
function OnStart()
	Agent:pushBehavior(idle)
	Agent:Init()

	Agent.model = Asset:LoadModel("Assets/Models/Temporary_Assets/suzanne.glb")
	--Agent.model = Asset:LoadModel("Assets/Models/Temporary_Assets/temp_Robot.fbx")
	Entity:AddComponent(EntityID, "Model", Agent.model)
	Entity:AddComponent(EntityID, "AgentStats", Agent.stats)
	Entity:AddComponent(EntityID, "BoxCollider", Vector3.New(1, 1, 1), true)
	Entity:AddComponent(EntityID, "Rigidbody", true)

end

function OnUpdate()
	Agent:doBehavior()
	Agent.pos = Vector3.FromTable(Entity:GetTransformPosData(EntityID))
end

function OnCollisionEnter(self, e1, e2)
	entity = e2
	if entity == EntityID then
		entity = e1
	end
	if Entity:HasComponent(entity, "Bullet") and Agent.stats.hp > 0.0 then
		if Entity:IsBulletLocal(e2) then
			print("Bullet is local")
			Agent:Damage(10)
		else
			print("Bullet is Not Local, Bullet from other player")
		end
		print(" Current HP: " .. Agent.stats.hp)

	else
		--print("Agent touched " .. entity .. " at  " .. Agent.pos)
		Agent:Collision(entity)
	end
end

