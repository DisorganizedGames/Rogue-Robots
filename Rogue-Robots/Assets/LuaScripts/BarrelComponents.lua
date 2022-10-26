require("VectorMath")

local BarrelComponents = {}

--[[
ARGUMENTS
1:
]]

function BarrelComponents:BasicBarrel()	
	return 
	{
		bulletModel = Asset:LoadModel("Assets/Models/Ammunition/Bullet/556x45_bullet.fbx"),
		gunShotSound = Asset:LoadAudio("Assets/Audio/TestShoot.wav"),
		speed = 75.0,
		size = Vector3.New(15.0, 15.0, 15.0),

		Update = function(self, gunEntity, parentEntityID, bullet)

			Entity:ModifyComponent(bullet.entity, "Transform", bullet.size + self.size, 3)

			Entity:AddComponent(bullet.entity, "Model", self.bulletModel)
			Entity:AddComponent(bullet.entity, "BoxCollider", Vector3.New(.1, .1, .1), true)
			Entity:AddComponent(bullet.entity, "Rigidbody", false)
			Entity:AddComponent(bullet.entity, "Bullet", parentEntityID)

			Entity:PlayAudio(gunEntity.entityID, self.gunShotSound, true)

			Physics:RBSetVelocity(bullet.entity, bullet.forward * (bullet.speed + self.speed))
		end,

		Destroy = function(self)
			return
		end
	}
end

function BarrelComponents:Grenade()
	return 
	{
		bulletModel = Asset:LoadModel("Assets/Models/Ammunition/Grenade/Grenade.fbx"),
		gunShotSound = Asset:LoadAudio("Assets/Audio/TestShoot.wav"),
		grenadeSpeed = 9.2,
		upSpeed = 5.0,
		grenadeSize = Vector3.New(0.2, 0.2, 0.2),
		waitForFire = 0.0,
		timeBetweenShots = 0.5,

		Update = function(self, gunEntity, parentEntityID, bullet)
			
			Entity:ModifyComponent(bullet.entity, "Transform", bullet.size * 0.02 + self.grenadeSize, 3)

			local up = Vector3.FromTable(Entity:GetUp(parentEntityID))
			local right = Vector3.FromTable(Entity:GetRight(parentEntityID))

			local angle = -math.pi / 2

			local newForward = RotateAroundAxis(Entity:GetForward(parentEntityID), right, angle)
			local newUp = RotateAroundAxis(up, right, angle)
			Entity:SetRotationForwardUp(bullet.entity, newForward, newUp)

			Entity:ModifyComponent(bullet.entity, "Transform", bullet.startPos, 1)

			Entity:AddComponent(bullet.entity, "Model", self.bulletModel)
			Entity:AddComponent(bullet.entity, "SphereCollider", 0.2, true)
			Entity:AddComponent(bullet.entity, "Rigidbody", false)
			Entity:AddComponent(bullet.entity, "Bullet", parentEntityID)

			Entity:PlayAudio(gunEntity.entityID, self.gunShotSound, true)

			Physics:RBSetVelocity(bullet.entity, bullet.forward * (bullet.speed + self.grenadeSpeed) + up * self.upSpeed)
		end,

		Destroy = function(self, bullet)
			local change = Length(bullet.size) * 0.1
			local power = 20.0 + change
			local radius = 5.0 + change
			Physics:Explosion(bullet.entity, power, radius)
		end,

		CreateBullet = function(self)
			if self.waitForFire < ElapsedTime then
				self.waitForFire = self.timeBetweenShots + ElapsedTime
				return true
			end
			return false
		end
	}
end

--Add more components here.

return BarrelComponents