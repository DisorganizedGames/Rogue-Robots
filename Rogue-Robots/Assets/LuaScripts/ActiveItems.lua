local ActiveItems = {}

ActiveItems.trampoline = {
	trampolineModel = Asset:LoadModel("Assets/Models/Temporary_Assets/Trampoline.glb"),

	activate = function(self, playerEntity)
		local trampolineEntity = Scene:CreateEntity(playerEntity)

		local pos = Vector3.FromTable(Entity:GetTransformPosData(playerEntity))
		local rotation = Vector3.Zero()
		local size = Vector3.New(1.0, 1.0, 1.0)

		local forward = Vector3.FromTable(Entity:GetForward(playerEntity))

		Entity:AddComponent(trampolineEntity, "Transform", pos + forward * 3.0, rotation, size)
		Entity:AddComponent(trampolineEntity, "Model", self.trampolineModel)
		Entity:AddComponent(trampolineEntity, "BoxColliderMass", Vector3.New(1, 0.5, 1), true, 100000.0)
		Entity:AddComponent(trampolineEntity, "Rigidbody", false)
		Physics:RBConstrainRotation(trampolineEntity, true, true, true)
		--Physics:RBConstrainPosition(trampolineEntity, true, false, true)
		Entity:AddComponent(trampolineEntity, "Script", "Trampoline.lua")
	end
}

return ActiveItems