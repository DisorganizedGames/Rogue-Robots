function Fact (n)
	if n == 0 then
		return 1
    else
        return n * Fact(n-1)
	end
end

function OnUpdate()
	Fact(100)
end