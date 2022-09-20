

function Die()
	print("Enemy died and dropped: ")
	EventSystem:InvokeEvent("EnemyDrop", {5, 6, 1})
end

