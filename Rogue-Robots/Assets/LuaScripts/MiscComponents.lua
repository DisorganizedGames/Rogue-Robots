require("VectorMath")

MiscComponent = {}

function MiscComponent.BasicShot()
	return 
	{
		pressed = false,

		Update = function(self, parentEntity)
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

			local pos = Vector3.FromTable(Entity:GetTransformPosData(parentEntity))
			local forward = Vector3.FromTable(Entity:GetForward(parentEntity))
			local up = Vector3.FromTable(Entity:GetUp(parentEntity))
			local right = Vector3.FromTable(Entity:GetRight(parentEntity))

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
		end
	}
end

function MiscComponent.FullAuto()
	return 
	{
		cooldown = 0,

		Update = function(self, parentEntity) 
			self.cooldown = self.cooldown - DeltaTime
			if self.cooldown <= 0.0 and Entity:GetAction(parentEntity, "Shoot") then
				self.cooldown = 0.1
				
				local pos = Vector3.FromTable(Entity:GetTransformPosData(parentEntity))
				local forward = Vector3.FromTable(Entity:GetForward(parentEntity))
				local up = Vector3.FromTable(Entity:GetUp(parentEntity))
				local right = Vector3.FromTable(Entity:GetRight(parentEntity))

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
		end
	}
end

function MiscComponent.ChargeShot()
	return 
	{
		shotPower = 0.0,
		chargeSpeed = 10.0,
		pressing = false,

		Update = function(self, parentEntity)
			
			if Entity:GetAction(parentEntity, "Shoot") then
				self.pressing = true
				self.shotPower = self.shotPower + self.chargeSpeed * DeltaTime

			elseif self.pressing then
				self.pressing = false
				
				local pos = Vector3.FromTable(Entity:GetTransformPosData(parentEntity))
				local forward = Vector3.FromTable(Entity:GetForward(parentEntity))
				local up = Vector3.FromTable(Entity:GetUp(parentEntity))
				local right = Vector3.FromTable(Entity:GetRight(parentEntity))

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
		end
	}
end

return MiscComponent

