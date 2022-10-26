require("VectorMath")

collided = false
setRB = false
tempAddVelocity = {}
deathTimer = 0.0
deathTime = 7.36

deathTimer = deathTime + ElapsedTime

function OnUpdate()
	for i = #tempAddVelocity, 1, -1 do
		Physics:RBSetVelocity(tempAddVelocity[i], Vector3.New(0.0, 15.0, 0.0))
		table.remove(tempAddVelocity, i)
	end

	if (deathTimer < ElapsedTime) then
		Entity:DestroyEntity(EntityID)
	end
end

function OnCollisionEnter(self, e1, e2)
	collided = true
	if (Entity:HasComponent(e2, "Rigidbody")) then
		--Currently we can't change physics value on OnCollisionEnter or OnCollisionExit
		table.insert(tempAddVelocity, e2)
	end
end