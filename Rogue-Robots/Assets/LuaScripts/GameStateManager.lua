local FinishedStateManager = {}

FinishedStateManager["GroupIDs"] = 0;
FinishedStateManager["FinishGroups"] = {}
FinishedStateManager["Tag"] = {}

function FinishedStateManager:IsFinished(tags)
	assert(type(tags) == "table", "Must be a collection of tags, i.e: { 'Tag1', 'Tag2' }")

	local finished = true

	for _, tag in pairs(tags) do
		local count = 0
		assert(not(FinishedStateManager["FinishGroups"][tag] == nil), "Tag doesn't exist")
		for _, evalFunc in pairs(FinishedStateManager["FinishGroups"][tag]) do
			local eval = evalFunc()
			assert(type(eval) == "boolean")
			finished = finished and eval
			count = count + 1
		end
		assert(not(count == 0), "No evaluations associated with the tag: " .. tag .. "'")
	end

	return finished
end


function FinishedStateManager:AddFinishCondition(tags, evalFunc)
	print("added")

	assert(type(tags) == "table", "Must be a collection of tags, i.e: { 'Tag1', 'Tag2' }")

	for k, tag in pairs(tags) do
		-- Init if doesn't exist
		if (FinishedStateManager["FinishGroups"][tag] == nil) then
			FinishedStateManager["FinishGroups"][tag] = {}
		end

		table.insert(FinishedStateManager["FinishGroups"][tag], evalFunc)
	end
end



function OnStart()
	print("Hello world from Game State Manager")

	function EventAddFinishCondition(...)
		print("FILIIIIPP")
		FinishedStateManager:AddFinishCondition(...)
	end

	EventSystem:Register("Game_AddFinishCondition", EventAddFinishCondition)

end


local once = true

function OnUpdate()
	if once then
		once = false
		--print("Shot: ", FinishedStateManager:IsFinished({ "Tag1" }))
	end

end

