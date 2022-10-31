require("VectorMath")
require("MiscComponents")

--Managers for objects & component functions.
local ObjectManager = require("Object")
local BarrelManager = require("BarrelComponents")
local MagazineManager = require("MagazineComponents")

--Tweakable values.
local InitialBulletSpeed = 75.0
local ShootCooldown = 0.1
local BulletDespawnDist = 50
local BulletSize = Vector3.New(3, 3, 3)

local savedBulletCount = 30
local currentAmmoCount = nil
local hasBasicBarrelEquipped = true

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
local currentAmmo = 30
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
	magazineComponent = MagazineManager.BasicEffect() --ObjectManager:CreateObject()

	currentAmmoCount = barrelComponent:GetMaxAmmo()

	EventSystem:Register("ItemPickup" .. EntityID, OnPickup)
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

	NormalBulletUpdate() -- Should be called something else, but necessary for bullet despawn

	if hasBasicBarrelEquipped then
		if (ReloadSystem()) then
			return
		end
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
		if createBullet and currentAmmoCount > 0 then

			if hasBasicBarrelEquipped then
				currentAmmo = currentAmmo - 1
			end

			CreateBulletEntity(newBullets[i], cameraEntity)
			barrelComponent:Update(gunEntity, EntityID, newBullets[i])
			--Keep track of which barrel created the bullet
			newBullets[i].barrel = barrelComponent
			magazineComponent:Update(newBullets[i])
			
			currentAmmoCount = currentAmmoCount - 1
			if currentAmmoCount == 0 and not hasBasicBarrelEquipped then
				barrelComponent = BarrelManager.BasicBarrel()
				Entity:RemoveComponent(EntityID, "BarrelComponent")
				Entity:AddComponent(EntityID, "BarrelComponent", 0, 30, 999999)
				currentAmmoCount = savedBulletCount
				hasBasicBarrelEquipped = true
			end
			Entity:UpdateMagazine(EntityID, currentAmmoCount)
		end
	end
	print("CurrentAmmoCount " .. currentAmmoCount)
	print("AmmoCount " .. currentAmmo)

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

		if hasBasicBarrelEquipped then
			currentAmmo = maxAmmo
			ammoLeft = -1
			Entity:UpdateMagazine(EntityID, maxAmmo)
			currentAmmoCount = maxAmmo
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

function OnPickup(pickup)
	if Entity:HasComponent(pickup, "BarrelComponent") then
		local typeOfPickedUpBarrel = Entity:GetBarrelType(pickup)
		local ammoCapForType = Entity:GetAmmoCapacityForBarrelType(pickup)
		local ammoPerPickup = Entity:GetAmmoCountPerPickup(pickup)

		local typeOfEquippedBarrel = Entity:GetBarrelType(EntityID)

		if typeOfPickedUpBarrel == typeOfEquippedBarrel then
				if currentAmmoCount + ammoPerPickup > ammoCapForType then
					currentAmmoCount = ammoCapForType
				else
					currentAmmoCount = currentAmmoCount + ammoPerPickup
				end
				Entity:UpdateMagazine(EntityID, currentAmmoCount)
		else
			--Type must be switched and things must be ''reset'', which we AS OF NOW simply do by replacing the component:
			Entity:RemoveComponent(EntityID, "BarrelComponent")
			if typeOfPickedUpBarrel == "Bullet" then --We can't pick it up in the world, but this makes it possible so we could.
				Entity:AddComponent(EntityID, "BarrelComponent", 0, 999999, ammoCapForType)
				barrelComponent = BarrelManager.BasicBarrel()
				hasBasicBarrelEquipped = true
			elseif typeOfPickedUpBarrel == "Grenade" then
				Entity:AddComponent(EntityID, "BarrelComponent", 1, ammoPerPickup, ammoCapForType)
				barrelComponent = BarrelManager.Grenade()
				hasBasicBarrelEquipped = false
			else
				Entity:AddComponent(EntityID, "BarrelComponent", 2, ammoPerPickup, ammoCapForType)
				barrelComponent = BarrelManager.Missile()
				hasBasicBarrelEquipped = false
			end
			savedBulletCount = currentAmmoCount
			currentAmmoCount = ammoPerPickup
		end
	elseif Entity:HasComponent(pickup, "MagazineModificationComponent") then
		local modificationType = Entity:GetModificationType(pickup)
		local activeModificationType = "None"
		if Entity:HasComponent(EntityID, "MagazineModificationComponent") then
			activeModificationType = Entity:GetModificationType(EntityID)
		end

		if activeModificationType == "None" then -- Add New
			if (modificationType == "Frost") then
				Entity:AddComponent(EntityID, "MagazineModificationComponent", 0) -- 0 is frost!
				magazineComponent = MagazineManager.FrostEffect()
			end
		elseif activeModificationType ~= modificationType then -- Let's dirty swap!
			Entity:RemoveComponent(EntityID, "MagazineModificationComponent")
			if modificationType == "Frost" then
				Entity:AddComponent(entityID, "MagazineModificationComponent", 0) -- 0 is frost!
				magazineComponent = MagazineManager.FrostEffect()
			end
		end
	end
end
