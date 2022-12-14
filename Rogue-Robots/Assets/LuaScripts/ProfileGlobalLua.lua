

function EmptyFunction()

end

function CallCppFunction()
	LuaToCppEmptyFunction()
end

function ForLoopAdd()
	sum = 0

	for i=1, 25 do
		for j=1, 25 do
			sum = sum + i % j
		end
	end
end