require("VectorMath")
require("MiscComponents")

local BarrelComponents = {}

--[[
ARGUMENTS
1:
]]

function SetPosition(bullet, transformEntity, upFactor, rightFactor, forwardFactor)
	local pos = Vector3.FromTable(Entity:GetTransformPosData(transformEntity))
	local forward = Vector3.FromTable(Entity:GetForward(transformEntity))
	local up = Vector3.FromTable(Entity:GetUp(transformEntity))
	local right = Vector3.FromTable(Entity:GetRight(transformEntity))

	pos = pos + up * upFactor + right * rightFactor + forward * forwardFactor

	Entity:ModifyComponent(bullet.entity, "Transform", pos, 1)
end

function BarrelComponents:BasicBarrel()	
	return 
	{
		bulletModel = Asset:LoadModel("Assets/Models/Ammunition/Bullet/556x45_bullet.gltf"),
		gunShotSound = Asset:LoadAudio("Assets/Audio/GunSounds/colt 45 si6025-2-1__R.wav"),
		speed = 340.0,
		size = Vector3.New(5.0, 5.0, 5.0),

		Update = function(self, gunEntity, parentEntityID, bullet, miscComponent, transformEntity)
			
			--Do not think the bullet models center is correct
			local boxColliderSize = bullet.size * 0.005 + Vector3.New(.1, .1, .1)
			Entity:ModifyComponent(bullet.entity, "Transform", bullet.size + self.size, 3)

			SetPosition(bullet, transformEntity, -0.04, 0.27, 0.9)
			local gunForward = Vector3.FromTable(Entity:GetRight(gunEntity.entityID))

			Entity:AddComponent(bullet.entity, "Model", self.bulletModel)
			Entity:AddComponent(bullet.entity, "BoxColliderMass", boxColliderSize, true, 0.075)
			Entity:AddComponent(bullet.entity, "Rigidbody", false)
			Entity:AddComponent(bullet.entity, "Bullet", parentEntityID, 35.0)

			Entity:PlayAudio(gunEntity.entityID, self.gunShotSound, true)

			Physics:RBSetVelocity(bullet.entity, gunForward * (bullet.speed + self.speed))
		end,

		Destroy = function(self)
			return
		end,

		GetMaxAmmo = function(self)
			return 30
		end,

		GetReloadTime = function(self)
			return 0.8
		end,

		GetECSType = function(self)
			return 0
		end,
	}
end

function BarrelComponents:Grenade()
	return 
	{
		bulletModel = Asset:LoadModel("Assets/Models/Ammunition/Grenade/Grenade.glb"),
		explosionModel = Asset:LoadModel("Assets/Models/Temporary_Assets/Explosion.glb"),
		gunShotSound = Asset:LoadAudio("Assets/Audio/GunSounds/grenade launcher pe23-4__R.wav"),
		grenadeSpeed = 9.2,
		upSpeed = 5.0,
		grenadeSize = Vector3.New(0.2, 0.2, 0.2),
		waitForFire = 0.0,
		timeBetweenShots = 0.5,

		Update = function(self, gunEntity, parentEntityID, bullet, miscComponent, transformEntity)
			
			local newGrenadeSize = bullet.size * 0.02 + self.grenadeSize
			local spherColliderRadius = bullet.size.x * 0.012 + self.grenadeSize.x
			Entity:ModifyComponent(bullet.entity, "Transform", newGrenadeSize, 3)

			local grenadeUp = Vector3.FromTable(Entity:GetUp(parentEntityID))

			SetPosition(bullet, transformEntity, -0.1, 0.27, 1.0)

			local gunForward = Vector3.FromTable(Entity:GetRight(gunEntity.entityID))

			Entity:AddComponent(bullet.entity, "Model", self.bulletModel)
			Entity:AddComponent(bullet.entity, "SphereCollider", spherColliderRadius, true)
			Entity:AddComponent(bullet.entity, "Rigidbody", false)

			Entity:PlayAudio(gunEntity.entityID, self.gunShotSound, true)

			Physics:RBSetVelocity(bullet.entity, bullet.forward * (bullet.speed + self.grenadeSpeed) + grenadeUp * self.upSpeed)
		end,

		Destroy = function(self, bullet, parentEntityID)
			local basePower = 20.0

			local change = Length(bullet.size) * 0.1
			local power = basePower + change
			local radius = 5.0 + change
			Physics:Explosion(bullet.entity, power, radius)
			
			explosionTrigger = Game:ExplosionEffect(bullet.entity, radius)

			Entity:AddComponent(explosionTrigger, "SphereTrigger", radius)
			Game:AddDamageToEntity(explosionTrigger, parentEntityID, 100.0 * power / basePower)
			Game:AddMagazineEffectsFromBullet(bullet.entity, explosionTrigger)

		end,

		CreateBullet = function(self, miscComponent)
			shotTime = self.timeBetweenShots

			if miscComponent.miscName == "FullAuto" then
				shotTime = shotTime / 2.0
			end

			if self.waitForFire < ElapsedTime then
				self.waitForFire = shotTime + ElapsedTime
				return true
			end
			return false
		end,

		GetMaxAmmo = function(self)
			return 20
		end,

		GetAmmoPerPickup = function(self)
			return 2
		end,

		GetECSType = function(self)
			return 1
		end,

		GetReloadTime = function(self)
			return 1.9
		end,
	}
end

function BarrelComponents:Missile()
	return 
	{
		bulletModel = Asset:LoadModel("Assets/Models/Ammunition/Missile/missile.glb"),
		gunShotMissileSound = Asset:LoadAudio("Assets/Audio/GunSounds/cannon pe24-2__R.wav"),
		waitForFire = 0.0,
		timeBetweenShots = 2.0,
		
		Update = function(self, gunEntity, parentEntityID, bullet, miscComponent, transformEntity)

			Entity:AddComponent(bullet.entity, "Model", self.bulletModel)

			local change = 1.0 + Length(bullet.size) * 0.04

			local homingMissileInfo = {}
			homingMissileInfo["explosionRadius"] = change
			homingMissileInfo["dmg"] = change
			
			if miscComponent.miscName == "FullAuto" then
				homingMissileInfo["startMotorSpeed"] = 90.0
				homingMissileInfo["mainMotorSpeed"] = 60.0
				homingMissileInfo["turnSpeed"] = 20.0
				homingMissileInfo["engineStartTime"] = 0.0
				homingMissileInfo["attackFlightPhaseStartTime"] = 0.0
				homingMissileInfo["homing"] = false
			end

			SetPosition(bullet, transformEntity, -0.1, 0.27, 1.0)

			Entity:AddComponent(bullet.entity, "HomingMissileComponent", parentEntityID, gunEntity.entityID, homingMissileInfo)
			Entity:PlayAudio(gunEntity.entityID, self.gunShotMissileSound, true)
		end,
		Destroy = function(self, bullet)
		end,
		CreateBullet = function(self)
			if self.waitForFire < ElapsedTime then
				self.waitForFire = self.timeBetweenShots + ElapsedTime
				return true
			end
			return false
		end,

		GetMaxAmmo = function(self)
			return 10
		end,

		GetAmmoPerPickup = function(self)
			return 1
		end,

		GetECSType = function(self)
			return 2
		end,

		GetReloadTime = function(self)
			return 3.0
		end,
	}
end



function BarrelComponents:Laser()
	return
	{
		gunShotLaserSound = Asset:LoadAudio("Assets/Audio/GunSounds/bazooka ss1-25__R.wav"),
		speed = 80.0,
		size = Vector3.New(1.0, 1.0, 1.0),

		Update = function(self, gunEntity, parentEntityID, bullet, miscComponent, transformEntity)
			local boxColliderSize = bullet.size * 0.005 + Vector3.New(.1, .1, .1)
			Entity:ModifyComponent(bullet.entity, "Transform", bullet.size + self.size, 3)

			SetPosition(bullet, transformEntity, -0.1, 0.27, 1.0)

			local gunForward = Vector3.FromTable(Entity:GetRight(gunEntity.entityID))

			Entity:AddComponent(bullet.entity, "LaserBullet", parentEntityID)
			Entity:AddComponent(bullet.entity, "BoxColliderMass", boxColliderSize, true, 0.075)
			Entity:AddComponent(bullet.entity, "Rigidbody", false)
			Entity:AddComponent(bullet.entity, "Bullet", parentEntityID, 100.0)

			Physics:RBSetVelocity(bullet.entity, gunForward * (bullet.speed + self.speed))

			Entity:PlayAudio(gunEntity.entityID, self.gunShotLaserSound, true)
		end,
		Destroy = function(self, bullet)
		end,
		CreateBullet = function(self)
			return true
		end,

		GetMaxAmmo = function(self)
			return 80
		end,

		GetAmmoPerPickup = function(self)
			return 20
		end,

		GetECSType = function(self)
			return 3
		end,

		GetReloadTime = function(self)
			return 1.0
		end,
	}
end


--Add more components here.

return BarrelComponents