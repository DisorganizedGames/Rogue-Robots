require("VectorMath")
require("MiscComponents")

--Managers for objects & component functions.
local ObjectManager = require("Object")
local BarrelManager = require("BarrelComponents")
local MagazineManager = require("MagazineComponents")

--The 3 different components.
--The pipeline is: misc -> barrel -> magazine.
local barrelComponent = nil
local magazineComponent = nil
local miscComponent = nil

-- TEMPORARY to make sure we don't over-switch components
local switched = false
local componentIdx = 0

local barrelSwitched = false
local barrelComponentIdx = 0

local magazineSwitched = false
local magazineComponentIdx = 0

--A template. Not supposed to be used, simply here to show what variables exist to be used.
local bulletTemplate = {
	entity = 0,					-- ID used by the ECS.
	forward = {},				-- Vector3 that describes the direction of the bullet.
	startPos = {},				-- Vector3 that describes the initial spawn position of the bullet.
	speed = 0, -- Float that describes the current speed of the bullet. 
	lifetime = 0,				-- Counter to know when to kill the bullet entity.
	size = {},					-- Vector3 that describes the scale of the bullet.
}

--Non-tweakable
local gunModel = nil
local bulletModel = nil
local gunShotSound = nil
local bullets = {}
local shootTimer = 0.0

local gunEntity = {
	entityID = nil,
	position = Vector3.Zero(),
	rotation = Vector3.Zero(),
}

local basicBarrelEquiped = true
--Ammo and reloading 
local maxAmmo = 30
local currentAmmo = 100000
local ammoLeft = -1
local reloadTimer = 0.0
local reloading = false
local reloadAngle = 0.0

function OnStart()
	gunModel = Asset:LoadModel("Assets/Models/Rifle/scene.gltf")
	bulletModel = Asset:LoadModel("Assets/Models/Ammunition/Bullet/556x45_bullet.fbx")
	gunShotSound = Asset:LoadAudio("Assets/Audio/TestShoot.wav")

	-- Initialize the gun view model entity
	gunID = Scene:CreateEntity(EntityID)
	gunEntity.entityID = gunID
	Entity:AddComponent(gunID, "Transform", gunEntity.position, gunEntity.rotation, {x=.15,y=.15,z=.15})
	Entity:AddComponent(gunID, "Model", gunModel)
	Entity:AddComponent(gunID, "Audio", gunShotSound, false, true)

	-- Initialize effect prefabs
	--MagazineManager:AddMaterial("FrostMaterial", Render:CreateMaterial({x=0.188, y=0.835, z=0.784}, 0.0, 0.0, { 0.0, 0.0, 0.0 }))


	-- Initialize base components
	miscComponent = MiscComponent.BasicShot()
	barrelComponent = BarrelManager.BasicBarrel()
	--barrelComponent = BarrelManager.Grenade()  --ObjectManager:CreateObject()
	magazineComponent = MagazineManager.BasicEffect() --ObjectManager:CreateObject()
end

local tempMode = 0
local tempTimer = 0.0
function OnUpdate()
	-- Update gun model position
	local cameraEntity = Entity:GetPlayerControllerCamera(EntityID)

	gunEntity.position = Vector3.FromTable(Entity:GetTransformPosData(cameraEntity))
	local playerUp = Vector3.FromTable(Entity:GetUp(cameraEntity))
	local playerForward = Vector3.FromTable(Entity:GetForward(cameraEntity))
	local playerRight = Vector3.FromTable(Entity:GetRight(cameraEntity))

	-- Move gun down and to the right 
	gunEntity.position = gunEntity.position + playerRight * 0.2 - playerUp * 0.2

	-- Rotate the weapon by 90 degrees pitch
	local angle = -math.pi / 2 
	local gunForward = RotateAroundAxis(playerForward, playerRight, angle)
	local gunUp = RotateAroundAxis(playerUp, playerRight, angle)

	Entity:SetRotationForwardUp(gunEntity.entityID, gunForward, gunUp)
	Entity:ModifyComponent(gunEntity.entityID, "Transform", gunEntity.position, 1)

	-- Switch misc component if we should
	-- THIS IS TEMPORARY
	if Entity:GetAction(EntityID, "SwitchComponent") and not switched then
		switched = true
		if componentIdx == 0 then
			miscComponent = MiscComponent.FullAuto()
			componentIdx = 1
		else
			miscComponent = MiscComponent.ChargeShot()
			componentIdx = 0
		end
	elseif not Entity:GetAction(EntityID, "SwitchComponent") then
		switched = false
	end
	--Barrel temp switch
	if Entity:GetAction(EntityID, "SwitchBarrelComponent") and not barrelSwitched then
		barrelSwitched = true
		if barrelComponentIdx == 0 then
			barrelComponent = BarrelManager.Grenade() 
			barrelComponentIdx = 1
			ammoLeft = 15
			basicBarrelEquiped = false
		elseif barrelComponentIdx == 1 then
			barrelComponent = BarrelManager.Missile()
			barrelComponentIdx = 2
			ammoLeft = 6
			basicBarrelEquiped = false
		else
			barrelComponent = BarrelManager.BasicBarrel()
			barrelComponentIdx = 0
			ammoLeft = -1
			basicBarrelEquiped = true
		end
	elseif not Entity:GetAction(EntityID, "SwitchBarrelComponent") then
		barrelSwitched = false
	end
	--Magazine temp switch
	if Entity:GetAction(EntityID, "SwitchMagazineComponent") and not magazineSwitched then
		magazineSwitched = true
		if magazineComponentIdx == 0 then
			magazineComponent = MagazineManager.FrostEffect() 
			magazineComponentIdx = 1
		else
			magazineComponent = MagazineManager.BasicEffect()
			magazineComponentIdx = 0
		end
	elseif not Entity:GetAction(EntityID, "SwitchMagazineComponent") then
		magazineSwitched = false
	end

	NormalBulletUpdate()

	if (ReloadSystem()) then
		return
	end

	-- Gun firing logic
	-- Returns a table of bullets
	local newBullets = miscComponent:Update(EntityID, cameraEntity)
	if not newBullets then
		return
	end
	for i=1, #newBullets do
		
		local createBullet = true
		if barrelComponent.CreateBullet then
			createBullet = barrelComponent:CreateBullet()
		end
		if createBullet and currentAmmo > 0 then
			currentAmmo = currentAmmo - 1

			CreateBulletEntity(newBullets[i], cameraEntity)
			barrelComponent:Update(gunEntity, EntityID, newBullets[i])
			--Keep track of which barrel created the bullet
			newBullets[i].barrel = barrelComponent
			magazineComponent:Update(newBullets[i])
		end
	end
	--NormalBulletUpdate()

	--if not bullet then
	--	return
	--end

	--NormalBulletSpawn(bullet)

	--BarrelComponent:Update(barrelComponent)
	--MagazineComponent:Update(magazineComponent)
end

function CreateBulletEntity(bullet, transformEntity)
	bullet.entity = Scene:CreateEntity(EntityID)
	table.insert(bullets, bullet)

	size = Vector3.New(1.0, 1.0, 1.0)
	if Vector3.Zero == bullet.size then
		size = bullet.size
	end

	Entity:AddComponent(bullet.entity, "Transform",
		Vector3.Zero(),
		Vector3.Zero(),
		size--bullet.size
	)
	local up = Vector3.FromTable(Entity:GetUp(transformEntity))
	local angle = -math.pi / 2

	local newForward = RotateAroundAxis(Entity:GetForward(transformEntity), up, angle)
	Entity:SetRotationForwardUp(bullet.entity, newForward, up)

	Entity:ModifyComponent(bullet.entity, "Transform", bullet.startPos, 1)
end

function ReloadSystem()
	if (Entity:HasComponent(EntityID, "ThisPlayer")) then
		Game:AmmoUI(currentAmmo, ammoLeft)
	end

	local oldMaxAmmo = maxAmmo
	maxAmmo = barrelComponent:GetMaxAmmo()
	if (maxAmmo ~= oldMaxAmmo) then
		currentAmmo = maxAmmo
	end

	--When reloading
	if reloadTimer >= ElapsedTime then
		--Reloading Animation 
		reloadAngle = reloadAngle + 2.0 * math.pi * DeltaTime / barrelComponent:GetReloadTime()

		local gunUp = Vector3.FromTable(Entity:GetUp(gunEntity.entityID))
		local gunForward = Vector3.FromTable(Entity:GetForward(gunEntity.entityID))
		local gunRight = Vector3.FromTable(Entity:GetRight(gunEntity.entityID))

		local newGunForward = RotateAroundAxis(gunForward, gunRight, reloadAngle)
		local newGunUp = RotateAroundAxis(gunUp, gunRight, reloadAngle)

		Entity:SetRotationForwardUp(gunEntity.entityID, newGunForward, newGunUp)
		Entity:ModifyComponent(gunEntity.entityID, "Transform", gunEntity.position, 1)

		return true
	end
	reloadAngle = 0.0

	if reloading then
		reloading = false
		local reloadAmount = maxAmmo - currentAmmo
		ammoLeft = ammoLeft - reloadAmount
		if ammoLeft < 0 then
			reloadAmount = ammoLeft + reloadAmount
			ammoLeft = 0
		end
		currentAmmo = currentAmmo + reloadAmount

		if basicBarrelEquiped then
			currentAmmo = maxAmmo
			ammoLeft = -1
		end
	end

	if Entity:GetAction(EntityID, "Reload") and currentAmmo < maxAmmo and (ammoLeft > 0 or ammoLeft == -1) then
		reloadTimer = barrelComponent:GetReloadTime() + ElapsedTime
		reloading = true
		return true
	end

	return false
end

--If there is not barrel component start.
function NormalBulletSpawn(bullet, transformEntity)
	bullet.entity = Scene:CreateEntity(EntityID)
	table.insert(bullets, bullet)

	Entity:AddComponent(bullet.entity, "Transform",
		Vector3.Zero(),
		Vector3.Zero(),
		bullet.size
	)
	
	local up = Vector3.FromTable(Entity:GetUp(transformEntity))
	local angle = -math.pi / 2

	local newForward = RotateAroundAxis(Entity:GetForward(transformEntity), up, angle)
	Entity:SetRotationForwardUp(bullet.entity, newForward, up)
	Entity:ModifyComponent(bullet.entity, "Transform", bullet.startPos, 1)

	Entity:AddComponent(bullet.entity, "Model", bulletModel)
	Entity:AddComponent(bullet.entity, "BoxCollider", Vector3.New(.1, .1, .1), true)
	Entity:AddComponent(bullet.entity, "Rigidbody", false)
	Entity:AddComponent(bullet.entity, "Bullet", EntityID)		-- Note: bullet damage is added in Lua interface

	Entity:PlayAudio(gunEntity.entityID, gunShotSound, true)

	Physics:RBSetVelocity(bullet.entity, bullet.forward * bullet.speed)
end

--If there is no barrel component update.
function NormalBulletUpdate()
	for i = #bullets, 1, -1 do -- Iterate through bullets backwards to make removal of elements safe
		bullets[i].lifetime = bullets[i].lifetime + DeltaTime
		if bullets[i].lifetime > 5.0 then
			bullets[i].barrel:Destroy(bullets[i], EntityID)
			Entity:DestroyEntity(bullets[i].entity)
			table.remove(bullets, i)
		end
	end
end