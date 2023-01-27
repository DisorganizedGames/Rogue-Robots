

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

function FactFunction()
	Fact(100)
end

function Fact (n)
	if n == 0 then
		return 1
    else
        return n * Fact(n-1)
	end
end

function BubbleSort()
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
