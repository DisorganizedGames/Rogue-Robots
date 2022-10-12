require("VectorMath")
local MiscManager = {}

local MiscComponent = {
	--Tweakable--
	--Normal Gun
	shootCooldown = 0.1,
	--Charge Shot
	chargeSpeed = 5.0,
	--Non-tweakable--
	--Normal Gun
	shootTimer = 0.0,
	--Charge Shot
	shotPower = 0.0,
	pressing = false
}

function MiscManager:CreateComponent(o)
	return MiscComponent:New(o)
end

function MiscComponent:New(o)
	o = o or {}
	setmetatable(o, self)
	self.__index = self

--Tweakable--
	--Normal Gun
	self.shootCooldown = 0.1
	--Charge Shot
	self.chargeSpeed = 5.0
	--Non-tweakable--
	--Normal Gun
	self.shootTimer = 0.0
	--Charge Shot
	self.shotPower = 0.0
	self.pressing = false
	return o
end

--[[
ARGUMENTS
1: barrel component
2: magazine component
3: bullets table
4: initial bullet speed
5: base size vector3
6: entity ID
]]

function MiscComponent:NormalGun(pos, ...)
	local args = {...}

	self.shootTimer = self.shootTimer - DeltaTime
	if  Entity:GetAction(args[6], 4) then
		--If the shoot cooldown is up and we are clicking.
		if self.shootTimer <= 0.0 and Entity:GetAction(args[6], 1) then
			self.shootTimer = self.shootCooldown

			local forward = Vector3.fromTable(Entity:GetForward(args[6]))
			local up = Vector3.fromTable(Entity:GetUp(args[6])) 

			local bullet = {}
			bullet.entity = Entity:CreateEntity()
			bullet.forward = forward
			bullet.startPos = pos
			bullet.speed = args[4]
			bullet.size = Vector3.new(2.0, 2.0, 2.0)
			bullet.lifetime = 0.0
			table.insert(args[3], bullet)

			local angle = -math.pi / 2
			local actualForward = RotateAroundAxis(forward, up, angle)

			Entity:AddComponent(bullet.entity, "Transform",{x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, {x = 0.0, y = 0.0, z = 0.0}, bullet.size)
			Entity:SetRotationForwardUp(bullet.entity, actualForward, up)
			Entity:ModifyComponent(bullet.entity, "Transform", {x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, 1)
			if args[1].OnStart then
				args[1]:OnStart()
			else
				EventSystem:InvokeEvent("NormalBulletSpawn" .. tostring(args[6]), bullet)
			end
		end
	
		if args[1].OnUpdate then
			args[1]:OnUpdate()
		else
			EventSystem:InvokeEvent("NormalBulletUpdate" .. tostring(args[6]))
		end
		if args[2].OnUpdate then
			args[2]:OnUpdate()
		end
	else
		if Entity:GetAction(args[6], 1) then
			self.pressing = true
			self.shotPower = self.shotPower + DeltaTime * self.chargeSpeed
		elseif self.pressing then --If we released the button.
			self.pressing = false
			local forward = Vector3.fromTable(Entity:GetForward(args[6]))
			local up = Vector3.fromTable(Entity:GetUp(args[6]))
		
			local bullet = {}
			bullet.entity = Entity:CreateEntity()
			bullet.forward = forward
			bullet.startPos = pos
			bullet.speed = args[4]
			bullet.lifetime = 0.0
			table.insert(args[3], bullet)

			local angle = -math.pi / 2
			local actualForward = RotateAroundAxis(forward, up, angle)

			Entity:AddComponent(bullet.entity, "Transform", {x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, {x = 0.0, y = 0.0, z = 0.0}, {x = args[5].x + self.shotPower, y = args[5].y + self.shotPower, z = args[5].z + self.shotPower})
			Entity:SetRotationForwardUp(bullet.entity, actualForward, up)
			Entity:ModifyComponent(bullet.entity, "Transform", {x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, 1)

			if args[1].OnStart then
				args[1]:OnStart()
			else
				EventSystem:InvokeEvent("NormalBulletSpawn" .. tostring(args[6]), bullet)
			end

			self.shotPower = 0.0
		end
	
		if args[1].OnUpdate then
			args[1]:OnUpdate()
		else
			EventSystem:InvokeEvent("NormalBulletUpdate" .. tostring(args[6]))
		end
		if args[2].OnUpdate then
			args[2]:OnUpdate()
		end
	end
end

--Add more components here.

return MiscManager