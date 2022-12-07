require("VectorMath")

collided = false
setRB = false
tempAddVelocity = {}
deathTimer = 0.0
deathTime = 7.36

deathTimer = deathTime + ElapsedTime

power = 22.0

trampolineSounds = {Asset:LoadAudio("Assets/Audio/Items/Tramp_1.wav"), Asset:LoadAudio("Assets/Audio/Items/Tramp_2.wav"), Asset:LoadAudio("Assets/Audio/Items/Tramp_3.wav")}

function OnStart()
	Entity:AddComponent(EntityID, "Audio", 0, false, true)
end

function OnUpdate()
	if #tempAddVelocity > 0 then
		math.randomseed(os.time())
		Entity:PlayAudio(EntityID, trampolineSounds[math.random(1, #trampolineSounds)], true)
	end

	for i = #tempAddVelocity, 1, -1 do
		Physics:RBSetVelocity(tempAddVelocity[i], Vector3.New(0.0, power, 0.0))
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