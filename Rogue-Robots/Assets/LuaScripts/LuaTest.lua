
print("Hello World I am in LUA")

print("hej")

m_number = 0

print(Input.LeftClick())

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

EventSystem:Register("Event", EventFunc)

EventSystem:InvokeEvent("Event", 20, "Hello Friend", {text = "table text"})
