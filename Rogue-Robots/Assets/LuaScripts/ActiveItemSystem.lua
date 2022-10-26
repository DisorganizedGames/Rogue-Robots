ActiveItems = require("ActiveItems")

activeItem = nil
clicked = false
activeEntityCreated = nil

function OnStart()
	activeItem = ActiveItems.trampoline
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
			activeItem = ActiveItems.trampoline
			clicked = true
		end
	else
		clicked = false
	end
end