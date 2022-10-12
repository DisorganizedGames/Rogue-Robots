require("VectorMath")

--Tweakable values.
local MaxAmmo = 10
local InitialBulletSpeed = 75.0
local ShootCooldown = 0.1
local BulletDespawnDist = 50
local BulletSize = {
	x = 10.0,
	y = 10.0,
	z = 10.0
}

--Managers for objects & component functions.
local ObjectManager = require("Object")
local MiscManager = require("MiscComponents")
local MyMiscComponents = MiscManager:CreateComponent()
local BarrelManager = require("BarrelComponents")
local MagazineManager = require("MagazineComponents")

--The 3 different components.
--The pipeline is: misc -> barrel -> magazine.
local barrelComponent = nil
local magazineComponent = nil
local miscComponent = nil

--A template. Not supposed to be used, simply here to show what variables exist to be used.
local bulletTemplate = {
	entity = 0, --ID used by the ECS.
	forward = {}, --Vector3 that describes the direction of the bullet.
	startPos = {}, --Vector3 that describes the initial spawn position of the bullet.
	speed = InitialBulletSpeed, --Float that describes the current speed of the bullet. 
	lifetime = 0 -- counter to know when to kill the bullet entity
}

--Non-tweakable
local gunModel = nil
local bulletModel = nil
local gunShotSound = nil
local bullets = {}
local shootTimer = 0.0

local gunEntity = {
	entityID = nil,
	position = {x=0,y=0,z=0},
	rotation = {x=3.14/2,y=0,z=0}
}

function OnStart()
	gunModel = Asset:LoadModel("Assets/Rifle/scene.gltf")
	bulletModel = Asset:LoadModel("Assets/556x45_bullet.fbx")
	gunShotSound = Asset:LoadAudio("Assets/Audio/TestShoot.wav")

	-- Initialize the gun view model entity
	gunID = Entity:CreateEntity()
	gunEntity.entityID = gunID
	Entity:AddComponent(gunID, "Transform", gunEntity.position, gunEntity.rotation, {x=.15,y=.15,z=.15})
	Entity:AddComponent(gunID, "Model", gunModel)

	barrelComponent = ObjectManager:CreateObject()
	magazineComponent = ObjectManager:CreateObject()
	miscComponent = MyMiscComponents
	miscComponent.OnUpdate = MyMiscComponents.ChargeShot

	--Events
	EventSystem:Register("NormalBulletUpdate" .. tostring(EntityID), NormalBulletUpdate) --Is called if there is no barrelcomponent.
	EventSystem:Register("NormalBulletSpawn" .. tostring(EntityID), NormalBulletSpawn) --Is called if there is no barrelcomponent.
end

local tempMode = 0
local tempTimer = 0.0
function OnUpdate()
	-- Update gun model position
	gunEntity.position = Vector3.fromTable(Entity:GetTransformPosData(EntityID))
	local playerUp = Vector3.fromTable(Entity:GetUp(EntityID))
	local playerForward = Vector3.fromTable(Entity:GetForward(EntityID))
	local playerRight = Vector3.fromTable(Entity:GetRight(EntityID))

	-- Move gun down and to the right 
	gunEntity.position = gunEntity.position + playerRight * 0.2 - playerUp * 0.2

	-- Rotate the weapon by 90 degrees pitch

	local angle = -math.pi / 2 -- 90 degrees
	local gunForward = RotateAroundAxis(playerForward, playerRight, angle)
	local gunUp = RotateAroundAxis(playerUp, playerRight, angle)

	Entity:SetRotationForwardUp(gunEntity.entityID, gunForward, gunUp)
	
	Entity:ModifyComponent(gunEntity.entityID, "Transform", gunEntity.position, 1)


	miscComponent.OnUpdate = MyMiscComponents.NormalGun

	miscComponent:OnUpdate(gunEntity.position + playerForward * 0.45 + playerUp * 0.06, barrelComponent, magazineComponent, bullets, InitialBulletSpeed, BulletSize, EntityID)
end

--If there is not barrel component start.
function NormalBulletSpawn(bullet)
	Entity:AddComponent(bullet.entity, "Model", bulletModel)
	Entity:AddComponent(bullet.entity, "Network")
	Entity:AddComponent(bullet.entity, "Audio", gunShotSound, true)
	Entity:AddComponent(bullet.entity, "BoxCollider", Vector3.new(.1, .1, .1), true)
	Entity:AddComponent(bullet.entity, "Rigidbody", false)
	Entity:AddComponent(bullet.entity, "Bullet")

	Physics:RBSetVelocity(bullet.entity, bullet.forward * bullet.speed)
end

--If there is no barrel component update.
function NormalBulletUpdate()
	for i = #bullets, 1, -1 do -- Iterate through bullets backwards to make removal of elements safe
		bullets[i].lifetime = bullets[i].lifetime + DeltaTime
		if bullets[i].lifetime > 5.0 then
			Entity:DestroyEntity(bullets[i].entity)
			table.remove(bullets, i)
		end
	end
end