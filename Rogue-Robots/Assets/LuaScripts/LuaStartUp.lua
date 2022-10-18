package.path = "Assets/LuaScripts/?.lua;" .. package.path

local FinishedStateManager = {}

FinishedStateManager["GroupIDs"] = 0;
FinishedStateManager["FinishGroups"] = {}
FinishedStateManager["Tag"] = {}


function FinishedStateManager:CreateFinishGroup(tag)
	local i = FinishedStateManager["GroupIDs"]
	FinishedStateManager["Tag"][i] = tag or "DefaultTag"
	FinishedStateManager["FinishGroups"][i] = {}
	FinishedStateManager["GroupIDs"] = FinishedStateManager["GroupIDs"] + 1
	return i
end

function FinishedStateManager:AddFinishCondition(group, evalFunc)
	table.insert(FinishedStateManager["FinishGroups"][group], evalFunc)
end




function FinishedStateManager:GroupFinished(group)
	local finished = true
	for _, evalFunc in pairs(FinishedStateManager["FinishGroups"][group]) do
		finished = finished and evalFunc()
	end
	return finished
end

function FinishedStateManager:AllGroupsFinished(tag)
	local finished = true
	local tagIn = tag or "DefaultTag"
	local count = 0
	for groupID, _ in pairs(FinishedStateManager["FinishGroups"]) do
		local gtag = FinishedStateManager["Tag"][groupID]
		if gtag == tagIn then
			finished = finished and FinishedStateManager:GroupFinished(groupID)
			count = count + 1
		end
	end
	if count == 0 then
		assert(false, "No finished groups with the tag: '" .. tag .. "'")
	end
	return finished
end







enemiesLeft = 0
tokensGot = 5

function EvalEnemiesWin()
	print("Eval Enemies")
	return enemiesLeft == 0
end

function EvalTokens()
	print("Eval Tokens")
	return tokensGot == 5
end

local win1 = EvalEnemiesWin
local win2 = EvalTokens


local a = FinishedStateManager:CreateFinishGroup()

FinishedStateManager:AddFinishCondition(a, win1)
FinishedStateManager:AddFinishCondition(a, win2)


p1Health = 20
p2Health = 20
p3Health = 70
p4Health = 21

minHealth = 20
function EvalHealth()
	return p1Health >= minHealth and p2Health >= minHealth and p3Health >= minHealth and p4Health >= minHealth
end

local win3 = EvalHealth

local b = FinishedStateManager:CreateFinishGroup()
FinishedStateManager:AddFinishCondition(b, win3)


print("Has won? ", FinishedStateManager:AllGroupsFinished())



