

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

function SendData()
	int, double, bool, returnString = LuaSendData(2, 1.3, true, "Hello I am a nice guy!")
	sum = int + double + string.len(returnString)
end