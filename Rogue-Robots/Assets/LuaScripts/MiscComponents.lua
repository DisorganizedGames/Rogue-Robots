require("VectorMath")

MiscComponent = {}

function MiscComponent:BasicShot()
	return 
	{
		miscName = "BasicShot",

		pressed = false,

		Update = function(self, parentEntity, transformEntity)
			spawnBullet = false
			if Entity:GetAction(parentEntity, "Shoot") then
				if not self.pressed then
					self.pressed = true
					spawnBullet = true
				end
			else
				self.pressed = false
			end

			if not spawnBullet then
				return nil
			end

			local pos = Vector3.FromTable(Entity:GetTransformPosData(transformEntity))
			local forward = Vector3.FromTable(Entity:GetForward(transformEntity))
			local up = Vector3.FromTable(Entity:GetUp(transformEntity))
			local right = Vector3.FromTable(Entity:GetRight(transformEntity))

			pos = pos + up * -0.14 + right * 0.2 + forward * 0.6

			local bullet = 
			{
				entity = 0,
				forward = forward,
				startPos = pos,
				speed = 0.0,
				--speed = 300.0,
				size = Vector3.Zero(),
				lifetime = 0
			}

			return {bullet} 
		end,

		GetECSType = function(self)
			return 0
		end,
	}
end

function MiscComponent:FullAuto()
	return 
	{
		miscName = "FullAuto",

		cooldown = 0,

		Update = function(self, parentEntity, transformEntity) 
			self.cooldown = self.cooldown - DeltaTime
			if self.cooldown <= 0.0 and Entity:GetAction(parentEntity, "Shoot") then
				self.cooldown = 0.1
				
				local pos = Vector3.FromTable(Entity:GetTransformPosData(transformEntity))
				local forward = Vector3.FromTable(Entity:GetForward(transformEntity))
				local up = Vector3.FromTable(Entity:GetUp(transformEntity))
				local right = Vector3.FromTable(Entity:GetRight(transformEntity))

				pos = pos + up * -0.14 + right * 0.2 + forward * 0.6

				local bullet = 
				{
					entity = 0,
					forward = forward,
					startPos = pos,
					speed = 0.0,
					--speed = 75.0,
					--size = Vector3.New(15, 15, 15),
					size = Vector3.Zero(),
					lifetime = 0
				}

				return {bullet} 
			end
			return nil
		end,

		GetECSType = function(self)
			return 1
		end,
	}
end

function MiscComponent:ChargeShot()
	return 
	{
		miscName = "ChargeShot",

		shotPower = 0.0,
		maxShotPower = 30.0,
		chargeSpeed = 10.0,
		pressing = false,

		chargeShotAudioEntity = -1,
		chargeShotSound = Asset:LoadAudio("Assets/Audio/GunSounds/ChargeShot.wav"),

		Update = function(self, parentEntity, transformEntity)
			
			if self.chargeShotAudioEntity == -1 then
				self.chargeShotAudioEntity = Scene:CreateEntity(parentEntity)
				Entity:AddComponent(self.chargeShotAudioEntity, "Audio", self.chargeShotSound, false, true)
				Entity:AddComponent(self.chargeShotAudioEntity, "Transform", Vector3:Zero(), Vector3:Zero(), Vector3:One())
				Entity:AddComponent(self.chargeShotAudioEntity, "Child", parentEntity, Vector3.Zero(), Vector3.Zero(), Vector3.One())
			end

			if Entity:GetAction(parentEntity, "Shoot") then
				self.pressing = true
				self.shotPower = self.shotPower + self.chargeSpeed * DeltaTime

				if self.maxShotPower < self.shotPower then
					self.shotPower = self.maxShotPower
				elseif not Entity:IsPlayingAudio(self.chargeShotAudioEntity) then
					Entity:PlayAudio(self.chargeShotAudioEntity, self.chargeShotSound, true)
				end

			elseif self.pressing then
				self.pressing = false
				
				Entity:StopAudio(self.chargeShotAudioEntity)

				local pos = Vector3.FromTable(Entity:GetTransformPosData(transformEntity))
				local forward = Vector3.FromTable(Entity:GetForward(transformEntity))
				local up = Vector3.FromTable(Entity:GetUp(transformEntity))
				local right = Vector3.FromTable(Entity:GetRight(transformEntity))

				pos = pos + up * -0.14 + right * 0.2 + forward * 0.6

				local bullet = 
				{
					entity = 0,
					forward = forward,
					startPos = pos,
					speed = 0.0,
					--speed = 75.0,
					--size = Vector3.New(10, 10, 10) + Vector3.New(self.shotPower, self.shotPower, self.shotPower),
					size = Vector3.New(self.shotPower, self.shotPower, self.shotPower),
					lifetime = 0
				}

				self.shotPower = 0.0

				return {bullet}

			end
			return nil
		end,

		GetECSType = function(self)
			return 2
		end,
	}
end

return MiscComponent

