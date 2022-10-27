local shrinkTime = 0.7
local explosionSize = Vector3.FromTable(Entity:GetTransformScaleData(EntityID))
local growTime = 0.13
local growingTimer = growTime + ElapsedTime
local growAcc = 0.0

local lightStrength = 160.0

local setTime = false
local destroyTime = growTime + shrinkTime + ElapsedTime

function OnUpdate()
	if not setTime then
		destroyTime = growTime + shrinkTime + ElapsedTime
		setTime = true
	end

	if destroyTime < ElapsedTime then
		Entity:DestroyEntity(EntityID)
		return
	end

	if growingTimer > ElapsedTime then
		growAcc = growAcc + DeltaTime / growTime
		Entity:ModifyComponent(EntityID, "Transform", explosionSize * growAcc, 3)
		Entity:ModifyComponent(EntityID, "Strength_PointLight", lightStrength * growAcc)
		return
	end

	local timeChange = (destroyTime - ElapsedTime)
	local shrink = timeChange * timeChange / (shrinkTime * shrinkTime)
	Entity:ModifyComponent(EntityID, "Transform", explosionSize * shrink, 3)
	Entity:ModifyComponent(EntityID, "Strength_PointLight", lightStrength * shrink)
end