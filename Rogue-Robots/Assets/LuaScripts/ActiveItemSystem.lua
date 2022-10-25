ActiveItems = require("ActiveItems")

activeItem = nil
clicked = false

function OnStart()
	activeItem = ActiveItems.trampoline
end


function OnUpdate()
	if not activeItem then
		return
	end

	if Entity:GetAction(EntityID, "ActivateActiveItem") and activeItem then
		if clicked == false then
			activeItem:activate(EntityID)
			activeItem = ActiveItems.trampoline
			clicked = true
		end
		--activeItem = nil
	else
		clicked = false
	end
end