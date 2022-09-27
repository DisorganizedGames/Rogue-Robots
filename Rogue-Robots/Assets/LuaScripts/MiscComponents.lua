local MiscComponents = {}

--[[
ARGUMENTS
1: barrel component
2: magazine component
3: bullets table
4: initial bullet speed
5: base size vector3
]]

--Tweakable
local chargeSpeed = 5.0
--Non-tweakable
local shotPower = 0.0
local pressing = false
function MiscComponents:ChargeShot(...)
	local args = {...}

	if Input:IsLeftPressed() then
		pressing = true
		
		shotPower = shotPower + DeltaTime * chargeSpeed
	elseif pressing then --If we released the button.
		pressing = false
		
		local bullet = {}
		bullet.entity = Entity:CreateEntity()
		bullet.forward = Player:GetForward()
		bullet.startPos = Player:GetPosition()
		bullet.speed = args[4]
		table.insert(args[3], bullet)

		Entity:AddComponent(bullet.entity, "Transform", {x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, {x = 0.0, y = 0.0, z = 0.0}, {x = args[5].x + shotPower, y = args[5].y + shotPower, z = args[5].z + shotPower})
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