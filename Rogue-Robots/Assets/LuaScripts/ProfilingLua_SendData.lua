

function OnUpdate()
	int, double, bool, returnString = LuaSendData(2, 1.3, true, "Hello I am a nice guy!")
	sum = int + double + string.len(returnString)
end