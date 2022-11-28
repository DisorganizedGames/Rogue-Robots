local ActiveItems = {}

ActiveItems.trampoline = {
	trampolineModel = Asset:LoadModel("Assets/Models/Temporary_Assets/Trampoline.glb"),

	activate = function(self, playerEntity)
		local trampolineEntity = Scene:CreateEntity(playerEntity)

		local pos = Vector3.FromTable(Entity:GetTransformPosData(playerEntity))
		local rotation = Vector3.Zero()
		local size = Vector3.New(0.5, 0.5, 0.5)

		local forward = Vector3.FromTable(Entity:GetForward(playerEntity))

		Entity:AddComponent(trampolineEntity, "Transform", pos + forward * 3.0, rotation, size)
		Entity:AddComponent(trampolineEntity, "Model", self.trampolineModel)
		Entity:AddComponent(trampolineEntity, "BoxColliderMass", Vector3.New(0.5, 0.25, 0.5), true, 100000.0)
		Entity:AddComponent(trampolineEntity, "Rigidbody", false)
		Physics:RBConstrainRotation(trampolineEntity, true, true, true)
		Entity:AddComponent(trampolineEntity, "Script", "Trampoline.lua")

		return trampolineEntity
	end,

	GetECSType = function(self)
			return 0
		end,


}



ActiveItems.turret = {
	turretBaseModelID = Asset:LoadModel("Assets/Models/Temporary_Assets/turretBase.glb"),
	turrtHeadModelID = Asset:LoadModel("Assets/Models/Temporary_Assets/turret2.glb"),

	activate = function(self, playerEntity)
		-- The Base has the root transform and will need to be placed at ground level and in the right direction
		local turretBase = Scene:CreateEntity(playerEntity)
		local pos = Vector3.FromTable(Entity:GetTransformPosData(playerEntity))

		local forward = Vector3.FromTable(Entity:GetForward(playerEntity))
		local up = Vector3.FromTable(Entity:GetUp(playerEntity))

		local spawnPos = pos + forward * 3.0
		hit, target = Physics:RayCast(spawnPos, spawnPos + Vector3.New(0.0, -2.0, 0.0))
		if(hit) then
			spawnPos = target
		end

		Entity:AddComponent(turretBase, "Transform", spawnPos, Vector3.Zero(), Vector3.New(1.0, 1.0, 1.0))
		Entity:AddComponent(turretBase, "Model", self.turretBaseModelID)
		Entity:SetRotationForwardUp(turretBase, forward, up)

		-- The head has all the turret components
		local turretHead = Scene:CreateEntity(playerEntity)
		Entity:AddComponent(turretHead, "Transform",  Vector3.Zero(), Vector3.Zero(), Vector3.One())
		Entity:AddComponent(turretHead, "Child", turretBase, Vector3.New(0.0, 1.0, 0.0), Vector3.Zero(), Vector3.One())
		Entity:AddComponent(turretHead, "Model", self.turrtHeadModelID)

		-- max range, yaw and pitch speed, yaw and pitch limits 
		Entity:AddComponent(turretHead, "TurretTargeting", 40.0, 1.5, 0.6, math.pi / 2, math.pi / 12)
		-- playerWhoOwns the turret, ammoCount, projectileSpeed, timeStep for fireRate, damage
		Entity:AddComponent(turretHead, "TurretBasicShooting", playerEntity, 200, 100.0, 0.2, 50.0, 1.6)


		return turretBase
	end,

	GetECSType = function(self)
			return 1 -- What should be returned here???
		end,


}

ActiveItems.reviver = {

	GetECSType = function(self)
			return 2
		end,


}

return ActiveItems