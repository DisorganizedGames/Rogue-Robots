EventSystem = {events = {}}

function EventSystem:Register(eventName, callBack)
	if self.events[eventName] == nil then
		self.events[eventName] = {callBack}
	else
		table.insert(self.events[eventName], callBack)
	end
end

function EventSystem:InvokeEvent(eventName, ...)
	--Check if there are any listeners registered to that event
	if self.events[eventName] == nil then
		print("Event: " .. eventName .. " does not exist!")
		return
	end

	for i=1, #self.events[eventName] do
		self.events[eventName][i](...)
	end
end

function EventSystem:UnRegister(eventName, callBack)
	--Check if there are any listeners registered to that event
	if self.events[eventName] == nil then
		print("Event: " .. eventName .. " does not exist!")
		return
	end

	for i=1, #self.events[eventName] do
		if self.events[eventName][i] == callBack then
			table.remove(self.events[eventName], i)
			break
		end
	end
end