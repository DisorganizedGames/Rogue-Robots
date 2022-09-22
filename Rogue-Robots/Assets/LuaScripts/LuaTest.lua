
print("Hello World I am in LUA")

print("hej")

m_number = 0

function SetNumber(number)
	print("Current Number", m_number)
	m_number = number
	print("Changed Number", m_number)
end

function OnUpdate()
	--print("Hello")
	return 20, 5.3, "Hello", {}, SetNumber, true
end

function EventFunc(number, string, table)
	print(number)
	print(string)
	print(table.text)
	print("Hello")
end

Table = {}
Table.fn = function (me, integer, float, double, bool, string, constchar, table, func, userData)
	print(integer)
	print(float)
	print(double)
	print(bool)
	print(string)
	print(constchar)
	print(table)
	print(func)
	print(userData)
	print(Table.Object)
	return integer, double, bool, string, table, func, userData
end

--t = !

--EventSystem:Register("Event", EventFunc)

--EventSystem:InvokeEvent("Event", 20, "Hello Friend", {text = "table text"})

--print(globalNumber)
--print(globalString)
--print(globalBool)
--print(GlobalFunction())
--print(globalTable)
--print(globalTable.tableNumber)
--LuaInterface:PrintCoolText()
--coolClass:PrintOtherText()
--print(globalTable.table.tableText)
