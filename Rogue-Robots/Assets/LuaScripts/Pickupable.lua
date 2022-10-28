
function OnCollisionEnter(self, e1, e2)
	if Entity:HasComponent(e2, "PlayerStats") then
		EventSystem:InvokeEvent("ItemPickup"..e2, e1)
		Entity:DestroyEntity(EntityID)
	end
end