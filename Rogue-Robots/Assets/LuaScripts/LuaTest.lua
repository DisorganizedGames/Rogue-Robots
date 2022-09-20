
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
	--return 20
end

function EventFunc(number, string, table)
	print(number)
	print(string)
	print(table.text)
	print("Hello")
end

--t = !

EventSystem:Register("Event", EventFunc)

EventSystem:InvokeEvent("Event", 20, "Hello Friend", {text = "table text"})

--print(globalNumber)
--print(globalString)
--print(globalBool)
--print(GlobalFunction())
--print(globalTable)
--print(globalTable.tableNumber)
--LuaInterface:PrintCoolText()
--coolClass:PrintOtherText()
--print(globalTable.table.tableText)
