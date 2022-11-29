ActiveItems = require("ActiveItems")

activeItem = nil
clicked = false
activeEntityCreated = nil

function OnStart()
	EventSystem:Register("ItemPickup" .. EntityID, OnPickup)
end

function OnUpdate()
	if not activeItem then
		return
	end

	if activeEntityCreated and not Entity:Exists(activeEntityCreated) then
		activeEntityCreated = nil
	end

	if Entity:GetAction(EntityID, "ActivateActiveItem") and activeItem then
		if clicked == false then
			if activeItem:GetECSType() ~= 2 then -- Dont activate reviver this way, ECS handles it
				activeEntityCreated = activeItem:activate(EntityID)
				activeItem = nil
				Entity:RemoveComponent(EntityID, "ActiveItem")
				clicked = true
			end
		end
	else
		clicked = false
	end
end

function OnPickup(pickup)
	local pickupTypeString = Entity:GetEntityTypeAsString(pickup)
	local playerID = EntityID

	if pickupTypeString == "Trampoline" or pickupTypeString == "Turret" or pickupTypeString == "Reviver" or pickupTypeString == "GoalRadar" then
		if Entity:HasComponent(playerID, "ActiveItem") then
			Entity:SpawnActiveItem(playerID)
			Entity:RemoveComponent(playerID, "ActiveItem")
		end
		if pickupTypeString == "Trampoline" then
			activeItem = ActiveItems.trampoline
			Entity:AddComponent(playerID, "ActiveItem", activeItem:GetECSType())
		end
		if pickupTypeString == "Turret" then
			activeItem = ActiveItems.turret
			Entity:AddComponent(playerID, "ActiveItem", activeItem:GetECSType())
		end
		if pickupTypeString == "Reviver" then
			activeItem = ActiveItems.reviver
			Entity:AddComponent(playerID, "ActiveItem", activeItem:GetECSType())
		end
		if pickupTypeString == "GoalRadar" then
			activeItem = ActiveItems.goalRadar
			Entity:AddComponent(playerID, "ActiveItem", activeItem:GetECSType())
		end
	end
end

function OnDestroy()
	EventSystem:UnRegister("ItemPickup" .. EntityID, OnPickup)
end