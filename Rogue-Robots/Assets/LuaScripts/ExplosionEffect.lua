local aliveTime = 0.7
local explosionSize = Vector3.FromTable(Entity:GetTransformScaleData(EntityID))
local growTime = 0.13
local growingTimer = growTime + ElapsedTime
local growAcc = 0.0

local destroyTime = growTime + aliveTime + ElapsedTime

function OnUpdate()
	if destroyTime < ElapsedTime then
		Entity:DestroyEntity(EntityID)
		return
	end

	if growingTimer > ElapsedTime then
		growAcc = growAcc + DeltaTime / growTime
		Entity:ModifyComponent(EntityID, "Transform", explosionSize * growAcc, 3)
		return
	end

	local timeChange = (destroyTime - ElapsedTime)
	local diff = timeChange * timeChange / (aliveTime * aliveTime)
	Entity:ModifyComponent(EntityID, "Transform", explosionSize * diff, 3)
end