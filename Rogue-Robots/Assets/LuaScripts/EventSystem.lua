EventSystem = {events = {}}

function EventSystem:Register(eventName, callBack)
	if self.events[eventName] == nil then
		self.events[eventName] = {callBack}
	else
		table.insert(self.events[eventName], callBack)
	end
end

function EventSystem:InvokeEvent(eventName, ...)
	if self.events[eventName] == nil then
		return
	end

	for i=1, #self.events[eventName] do
		self.events[eventName][i](...)
	end
end