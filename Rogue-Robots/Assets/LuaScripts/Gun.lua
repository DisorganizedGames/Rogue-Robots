--Tweakable values.
local a = {}
local MaxAmmo = 10
local InitialBulletSpeed = 10.0
local ShootCooldown = 1.0
local BulletDespawnDist = 50

--A template. Not supposed to be used, simply here to show what variables exist to be used.
local bulletTemplate = {
	entity = 0, --ID used by the ECS.
	forward = {}, --Vector3 that describes the direction of the bullet.
	startPos = {}, --Vector3 that describes the initial spawn position of the bullet.
	speed = InitialBulletSpeed --Float that describes the current speed of the bullet. 
}

local bulletModel = 0
local bullets = {}
local shootTimer = 0.0

local ObjectManager = require("Object")

--The pipeline is: misc -> barrel -> magazine.
local barrelComponent = nil
local magazineComponent = nil
local miscComponent = nil

function OnStart()
	bulletModel = Asset:LoadModel("Assets/red_cube.glb") --Load the bullet's model

	barrelComponent = ObjectManager:CreateObject()
	magazineComponent = ObjectManager:CreateObject()
	miscComponent = ObjectManager:CreateObject()
	miscComponent.OnUpdate = ChargeShot
end

function OnUpdate()
	shootTimer = shootTimer - DeltaTime

	if miscComponent.OnUpdate then
		miscComponent.OnUpdate()
	else
		NormalGunUpdate()
	end
end

--If there is no misc component.
function NormalGunUpdate()
	--If the shoot cooldown is up and we are clicking.
	if shootTimer < 0.0 and Input:IsLeftPressed() then
		shootTimer = ShootCooldown

		local bullet = {}
		bullet.entity = Entity:CreateEntity()
		bullet.forward = Player:GetForward()
		bullet.startPos = Player:GetPosition()
		bullet.speed = InitialBulletSpeed
		table.insert(bullets, bullet)

		Entity:AddComponent(bullet.entity, "Transform",{x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, {x = 0.0, y = 0.0, z = 0.0}, {x = 0.2, y = 0.2, z = 0.2})
		if barrelComponent.OnUpdate then
			barrelComponent.OnUpdate()
		else
			NormalBulletSpawn(bullet)
		end
	end
	
	if barrelComponent.OnUpdate then
		barrelComponent.OnUpdate()
	else
		NormalBulletUpdate()
	end
	if magazineComponent.OnUpdate then
		magazineComponent.OnUpdate()
	end
end

--If there is not barrel component.
function NormalBulletSpawn(bullet)
	Entity:AddComponent(bullet.entity, "Model", bulletModel)
	Entity:AddComponent(bullet.entity, "Network")
end

--If there is no magazine component
function NormalBulletUpdate()
	local i = 1
	while i ~= #bullets + 1 do
		local t = Entity:GetTransformPosData(bullets[i].entity)
		local dist = math.sqrt((t.x - bullets[i].startPos.x)^2 + (t.y - bullets[i].startPos.y)^2 + (t.z - bullets[i].startPos.z)^2)
		if dist > BulletDespawnDist then
			Entity:DestroyEntity(bullets[i].entity)
			table.remove(bullets, i)
			i = i - 1
		else
			Entity:ModifyComponent(bullets[i].entity, "Transform", {x = t.x + bullets[i].speed * DeltaTime * -bullets[i].forward.x, y = t.y + bullets[i].speed * DeltaTime * -bullets[i].forward.y, z = t.z + bullets[i].speed * DeltaTime * -bullets[i].forward.z}, 1)
			bullets[i].speed = bullets[i].speed - 0.9 * DeltaTime --temp
		end

		i = i + 1
	end
end

--Misc component.
local shotPower = 0.0
local chargeSpeed = 5.0
local pressing = false
function ChargeShot()
	if Input:IsLeftPressed() then
		pressing = true
		
		shotPower = shotPower + DeltaTime * chargeSpeed
	elseif pressing then --If we released the button.
		pressing = false
		
		local bullet = {}
		bullet.entity = Entity:CreateEntity()
		bullet.forward = Player:GetForward()
		bullet.startPos = Player:GetPosition()
		bullet.speed = InitialBulletSpeed
		table.insert(bullets, bullet)

		Entity:AddComponent(bullet.entity, "Transform", {x = bullet.startPos.x, y = bullet.startPos.y, z = bullet.startPos.z}, {x = 0.0, y = 0.0, z = 0.0}, {x = 0.2 + shotPower, y = 0.2 + shotPower, z = 0.2 + shotPower})
		if barrelComponent.OnStart then
			barrelComponent.OnStart()
		else
			NormalBulletSpawn(bullet)
		end

		shotPower = 0.0
	end
	
	if barrelComponent.OnUpdate then
		barrelComponent.OnUpdate()
	else
		NormalBulletUpdate()
	end
	if magazineComponent.OnUpdate then
		magazineComponent.OnUpdate()
	end
end