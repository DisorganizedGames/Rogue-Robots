local MiscComponents = {}

--[[
ARGUMENTS
1: barrel component
2: magazine component
3: bullets table
4: initial bullet speed
5: base size vector3
]]

-- Local to Normal gun
local shootTimer = 0.0
local ShootCooldown = 0.1
function MiscComponents:NormalGun(pos, ...)
	local args = {...}

	shootTimer = shootTimer - DeltaTime

	--If the shoot cooldown is up and we are clicking.
	if shootTimer <= 0.0 and Input:IsLeftPressed() then
		shootTimer = ShootCooldown

		local forward = Vector3.fromTable(Player:GetForward())
		local up = Vector3.fromTable(Player:GetUp()) 

		local bullet = {}
		bullet.entity = Entity:CreateEntity()
		bullet.forward = forward
		bullet.startPos = pos
		bullet.speed = args[4]
		bullet.size = Vector3.new(2.0, 2.0, 2.0)
		table.insert(args[3], bullet)

		local angle = 3.141592 / 2
		local actualForward = RotateAroundAxis(forward, up, angle)

		Entity:AddComponent(bullet.entity, "Transform",{x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, {x = 0.0, y = 0.0, z = 0.0}, bullet.size)
		Entity:SetRotationForwardUp(bullet.entity, actualForward, up)
		Entity:ModifyComponent(bullet.entity, "Transform", {x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, 1)
		if args[1].OnStart then
			args[1]:OnStart()
		else
			EventSystem:InvokeEvent("NormalBulletSpawn", bullet)
		end
	end
	
	if args[1].OnUpdate then
		args[1]:OnUpdate()
	else
		EventSystem:InvokeEvent("NormalBulletUpdate")
	end
	if args[2].OnUpdate then
		args[2]:OnUpdate()
	end
end

--Tweakable
local chargeSpeed = 5.0
--Non-tweakable
local shotPower = 0.0
local pressing = false
function MiscComponents:ChargeShot(pos, ...)
	local args = {...}

	if Input:IsLeftPressed() then
		pressing = true
		
		shotPower = shotPower + DeltaTime * chargeSpeed
	elseif pressing then --If we released the button.
		pressing = false

		local forward = Vector3.fromTable(Player:GetForward())
		local up = Vector3.fromTable(Player:GetUp())
		
		local bullet = {}
		bullet.entity = Entity:CreateEntity()
		bullet.forward = forward
		bullet.startPos = pos
		bullet.speed = args[4]
		table.insert(args[3], bullet)

		local angle = 3.141592 / 2
		local actualForward = RotateAroundAxis(forward, up, angle)

		Entity:AddComponent(bullet.entity, "Transform", {x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, {x = 0.0, y = 0.0, z = 0.0}, {x = args[5].x + shotPower, y = args[5].y + shotPower, z = args[5].z + shotPower})
		Entity:SetRotationForwardUp(bullet.entity, actualForward, up)
		Entity:ModifyComponent(bullet.entity, "Transform", {x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, 1)

		if args[1].OnStart then
			args[1]:OnStart()
		else
			EventSystem:InvokeEvent("NormalBulletSpawn", bullet)
		end

		shotPower = 0.0
	end
	
	if args[1].OnUpdate then
		args[1]:OnUpdate()
	else
		EventSystem:InvokeEvent("NormalBulletUpdate")
	end
	if args[2].OnUpdate then
		args[2]:OnUpdate()
	end
end

--Add more components here.

return MiscComponents