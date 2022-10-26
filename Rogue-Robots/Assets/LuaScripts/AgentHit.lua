
function OnCollisionEnter(self, e1, e2)
	bullet = e2
	if bullet == EntityID then
		bullet = e1
	end
	if Entity:HasComponent(bullet, "Bullet") then
		if Entity:IsBulletLocal(bullet) then
			Entity:AgentHit(EntityID, bullet)
		end
	end
end
