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

		Update = function(self, gunEntity, parentEntityID, bullet)

			Entity:AddComponent(bullet.entity, "Model", self.bulletModel)
			Entity:AddComponent(bullet.entity, "BoxCollider", Vector3.New(.1, .1, .1), true)
			Entity:AddComponent(bullet.entity, "Rigidbody", false)
			Entity:AddComponent(bullet.entity, "Bullet")

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

		Update = function(self, gunEntity, parentEntityID, bullet)
			
			Entity:ModifyComponent(bullet.entity, "Transform", bullet.size * 0.01, 3)

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
			Entity:AddComponent(bullet.entity, "Bullet")

			Entity:PlayAudio(gunEntity.entityID, self.gunShotSound, true)

			Physics:RBSetVelocity(bullet.entity, bullet.forward * (bullet.speed + self.grenadeSpeed) + up * self.upSpeed)
		end,

		Destroy = function(self, bullet)
			Physics:Explosion(bullet.entity, 10.0, 5.0)
		end
	}
end

--Add more components here.

return BarrelComponents