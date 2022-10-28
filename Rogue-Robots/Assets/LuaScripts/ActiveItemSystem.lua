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
		if clicked == false and not activeEntityCreated then
			activeEntityCreated = activeItem:activate(EntityID)
			activeItem = nil
			Entity:RemoveComponent(EntityID, "ActiveItem")
			clicked = true
		end
	else
		clicked = false
	end
end

function OnPickup(pickup)
	if Entity:HasComponent(pickup, "ActiveItem") then
		local type = Entity:GetActiveType(pickup)
		if Entity:HasComponent(EntityID, "ActiveItem") then
			Entity:RemoveComponent(EntityID, "ActiveItem")
		end
		if type == "Trampoline" then
			Entity:AddComponent(EntityID, "ActiveItem", 0) -- '0' is the trampoline enum!
			activeItem = ActiveItems.trampoline
		end
	end
end