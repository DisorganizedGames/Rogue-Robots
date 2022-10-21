require("VectorMath")
require("MiscComponents")

--Tweakable values.
local MaxAmmo = 10
local InitialBulletSpeed = 75.0
local ShootCooldown = 0.1
local BulletDespawnDist = 50
local BulletSize = Vector3.New(3, 3, 3)

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

--A template. Not supposed to be used, simply here to show what variables exist to be used.
local bulletTemplate = {
	entity = 0,					-- ID used by the ECS.
	forward = {},				-- Vector3 that describes the direction of the bullet.
	startPos = {},				-- Vector3 that describes the initial spawn position of the bullet.
	speed = InitialBulletSpeed, -- Float that describes the current speed of the bullet. 
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

	-- Initialize base components
	miscComponent = MiscComponent.BasicShot()
	--barrelComponent = BarrelManager.BasicBarrel() 
	barrelComponent = BarrelManager.Grenade()  --ObjectManager:CreateObject()
	magazineComponent = MagazineManager.BasicEffect()--ObjectManager:CreateObject()
end

local tempMode = 0
local tempTimer = 0.0
function OnUpdate()
	-- Update gun model position
	gunEntity.position = Vector3.FromTable(Entity:GetTransformPosData(EntityID))
	local playerUp = Vector3.FromTable(Entity:GetUp(EntityID))
	local playerForward = Vector3.FromTable(Entity:GetForward(EntityID))
	local playerRight = Vector3.FromTable(Entity:GetRight(EntityID))

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

	NormalBulletUpdate()

	-- Gun firing logic
	-- Returns a table of bullets
	local newBullets = miscComponent:Update(EntityID)
	if not newBullets then
		return
	end
	for i=1, #newBullets do
		
		local createBullet = true
		if barrelComponent.CreateBullet then
			createBullet = barrelComponent:CreateBullet()
		end
		if createBullet then
			CreateBulletEntity(newBullets[i])
			barrelComponent:Update(gunEntity, EntityID, newBullets[i])
			magazineComponent:Update()
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

function CreateBulletEntity(bullet)
	bullet.entity = Entity:CreateEntity()
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
	local up = Vector3.FromTable(Entity:GetUp(EntityID))
	local angle = -math.pi / 2

	local newForward = RotateAroundAxis(Entity:GetForward(EntityID), up, angle)
	Entity:SetRotationForwardUp(bullet.entity, newForward, up)

	Entity:ModifyComponent(bullet.entity, "Transform", bullet.startPos, 1)
end

--If there is not barrel component start.
function NormalBulletSpawn(bullet)
	bullet.entity = Scene:CreateEntity(EntityID)
	table.insert(bullets, bullet)

	Entity:AddComponent(bullet.entity, "Transform",
		Vector3.Zero(),
		Vector3.Zero(),
		bullet.size
	)
	
	local up = Vector3.FromTable(Entity:GetUp(EntityID))
	local angle = -math.pi / 2

	local newForward = RotateAroundAxis(Entity:GetForward(EntityID), up, angle)
	Entity:SetRotationForwardUp(bullet.entity, newForward, up)
	Entity:ModifyComponent(bullet.entity, "Transform", bullet.startPos, 1)

	Entity:AddComponent(bullet.entity, "Model", bulletModel)
	Entity:AddComponent(bullet.entity, "BoxCollider", Vector3.New(.1, .1, .1), true)
	Entity:AddComponent(bullet.entity, "Rigidbody", false)
	Entity:AddComponent(bullet.entity, "Bullet")

	Entity:PlayAudio(gunEntity.entityID, gunShotSound, true)

	Physics:RBSetVelocity(bullet.entity, bullet.forward * bullet.speed)
end

--If there is no barrel component update.
function NormalBulletUpdate()
	for i = #bullets, 1, -1 do -- Iterate through bullets backwards to make removal of elements safe
		bullets[i].lifetime = bullets[i].lifetime + DeltaTime
		if bullets[i].lifetime > 5.0 then
			barrelComponent:Destroy(bullets[i])
			Entity:DestroyEntity(bullets[i].entity)
			table.remove(bullets, i)
		end
	end
end