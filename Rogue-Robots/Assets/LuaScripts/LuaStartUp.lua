package.path = "Assets/LuaScripts/?.lua;" .. package.path

local GameStateManager = {}

GameStateManager["GroupIDs"] = 0;
GameStateManager["WinGroups"] = {}
GameStateManager["LoseGroups"] = {}

function GameStateManager:CreateWinConditionGroup()
	local i = GameStateManager["GroupIDs"]
	GameStateManager["WinGroups"][i] = {}
	GameStateManager["GroupIDs"] = GameStateManager["GroupIDs"] + 1
	return i
end

function GameStateManager:AddWinCondition(group, evalFunc)
	print("XDD\n")
	print(group)
	table.insert(GameStateManager["WinGroups"][group], evalFunc)
end





local a = GameStateManager.CreateWinConditionGroup()
local b = GameStateManager.CreateWinConditionGroup()


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

GameStateManager:AddWinCondition(a, win1)
GameStateManager:AddWinCondition(a, win2)


p1Health = 19
p2Health = 20
p3Health = 70
p4Health = 21

function EvalHealth()
	return p1Health >= 20 and p2Health >= 20 and p3Health >= 20 and p4Health >= 20
end

local win3 = EvalHealth

GameStateManager:AddWinCondition(b, win3)

local won = true
for _, v in pairs(GameStateManager["WinGroups"]) do

	-- Group
	local groupWon = true
	for _, evalFunc in pairs(v) do
		groupWon = groupWon and evalFunc()
	end
	won = won and groupWon
end

print("Has won? ", won)


