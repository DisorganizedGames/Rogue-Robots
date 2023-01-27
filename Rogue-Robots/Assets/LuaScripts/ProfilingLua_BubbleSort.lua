
function OnUpdate()
	a = {10, 5, 7, 6, 2, 1, 15, 3, 4, 6, 20}
	for i=1, #a do
		for j=i+1, #a do
			if a[j] < a[i] then
				temp = a[i]
				a[i] = a[j]
				a[j] = temp
			end
		end
	end
end