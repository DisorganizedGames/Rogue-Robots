#include "LuaW.h"
#include "LuaTable.h"

namespace DOG
{
	LuaW LuaW::s_luaW;

	LuaW::LuaW(lua_State* l)
	{
		m_luaState = l;
	}

	LuaW::LuaW()
	{
		//Create the luaState and add the libs
		m_luaState = luaL_newstate();
		luaL_openlibs(m_luaState);
	}

	void LuaW::Error(const std::string& errorMessage)
	{
		std::cout << "ERROR: " << errorMessage << "\n";
		PrintStack();
		//Clear the stack for safety
		ClearStack();
	}

	void LuaW::RunScript(const std::string& luaFileName)
	{
		const int getErrorMessage = -1;

		//Load lua file and run it
		int error = luaL_loadfile(m_luaState, luaFileName.c_str()) || lua_pcall(m_luaState, 0, LUA_MULTRET, 0);
		if (error)
		{
			Error(lua_tostring(m_luaState, getErrorMessage));
		}
	}

	//Sets an integer to a global variable
	void LuaW::PushGlobalNumber(const std::string& luaGlobalName, int number)
	{
		PushIntegerToStack(number);
		SetGlobal(luaGlobalName);
	}

	//Sets an float to a global variable
	void LuaW::PushGlobalNumber(const std::string& luaGlobalName, float number)
	{
		PushFloatToStack(number);
		SetGlobal(luaGlobalName);
	}

	//Sets an double to a global variable
	void LuaW::PushGlobalNumber(const std::string& luaGlobalName, double number)
	{
		PushDoubleToStack(number);
		SetGlobal(luaGlobalName);
	}

	//Sets an string to a global variable
	void LuaW::PushGlobalString(const std::string& luaGlobalName, const std::string& string)
	{
		PushStringToStack(string);
		SetGlobal(luaGlobalName);
	}

	//Sets an string to a global variable
	void LuaW::PushGlobalString(const std::string& luaGlobalName, const char* string)
	{
		PushStringToStack(string);
		SetGlobal(luaGlobalName);
	}

	//Sets an bool to a global variable
	void LuaW::PushGlobalBool(const std::string& luaGlobalName, bool boolean)
	{
		PushBoolToStack(boolean);
		SetGlobal(luaGlobalName);
	}

	//Create an interface for lua
	RegisterClassFunctions LuaW::CreateLuaInterface(const std::string& interfaceName)
	{
		RegisterClassFunctions reg;
		reg.className = interfaceName;
		return reg;
	}

	//Pushes the interface to lua
	void LuaW::PushLuaInterface(RegisterClassFunctions& registerInterface)
	{
		const int topStackItemIndex = -1;

		std::vector<luaL_Reg> luaRegs;

		for (int i = 0; i < registerInterface.classFunctions.size(); ++i)
		{
			luaRegs.push_back({ registerInterface.classFunctions[i].functionName.c_str(), registerInterface.classFunctions[i].classFunction });
		}
		//Have to have null, null at the end!
		luaRegs.push_back({ NULL, NULL });

		//Create a metatable on the stack (an table)
		luaL_newmetatable(m_luaState, registerInterface.className.c_str());
		//Sets the functions to the metatable
		luaL_setfuncs(m_luaState, luaRegs.data(), 0);
		//Copies the metatable on the stack
		lua_pushvalue(m_luaState, topStackItemIndex);
		//Sets the __index to the metatable (to it self)
		lua_setfield(m_luaState, topStackItemIndex, "__index");
		//Create the interface for lua
		SetGlobal(registerInterface.className);
	}

	void LuaW::PrintStack()
	{
		std::cout << "Stack:\n";
		int top = lua_gettop(s_luaW.m_luaState);
		for (int i = 1; i <= top; i++)
		{
			printf("%d\t%s\t", i, luaL_typename(s_luaW.m_luaState, i));
			switch (lua_type(s_luaW.m_luaState, i))
			{
			case LUA_TNUMBER:
				std::cout << lua_tonumber(s_luaW.m_luaState, i) << std::endl;
				break;
			case LUA_TSTRING:
				std::cout << lua_tostring(s_luaW.m_luaState, i) << std::endl;
				break;
			case LUA_TBOOLEAN:
				std::cout << (lua_toboolean(s_luaW.m_luaState, i) ? "true" : "false") << std::endl;
				break;
			case LUA_TNIL:
				std::cout << "nil" << std::endl;
				break;
			default:
				std::cout << lua_topointer(s_luaW.m_luaState, i) << std::endl;
				break;
			}
		}
	}

	//Pushes the global variable to the stack and then return it to the user
	int LuaW::GetGlobalInteger(const std::string& luaGlobalName)
	{
		GetGlobal(luaGlobalName);
		return GetIntegerFromStack();
	}

	//Pushes the global variable to the stack and then return it to the user
	float LuaW::GetGlobalFloat(const std::string& luaGlobalName)
	{
		GetGlobal(luaGlobalName);
		return GetFloatFromStack();
	}

	//Pushes the global variable to the stack and then return it to the user
	double LuaW::GetGlobalDouble(const std::string& luaGlobalName)
	{
		GetGlobal(luaGlobalName);
		return GetDoubleFromStack();
	}

	//Pushes the global variable to the stack and then return it to the user
	std::string LuaW::GetGlobalString(const std::string& luaGlobalName)
	{
		GetGlobal(luaGlobalName);
		return std::move(GetStringFromStack());
	}

	//Pushes the global variable to the stack and then return it to the user
	bool LuaW::GetGlobalBool(const std::string& luaGlobalName)
	{
		GetGlobal(luaGlobalName);
		return GetBoolFromStack();
	}

	//Pushes the global variable to the stack and then return it to the user
	Function LuaW::GetGlobalFunction(const std::string& luaGlobalName)
	{
		GetGlobal(luaGlobalName);
		return GetFunctionFromStack();
	}

	//Pushes the global variable to the stack and then return it to the user
	Table LuaW::GetGlobalTable(const std::string& luaGlobalName)
	{
		GetGlobal(luaGlobalName);
		return GetTableFromStack();
	}

	//Pushes the global variable to the stack and then return it to the user
	UserData LuaW::GetGlobalUserData(const std::string& luaGlobalName)
	{
		GetGlobal(luaGlobalName);
		return GetUserDataFromStack();
	}

	//Checks if the current index on the stack is an integer and return it
	int LuaW::GetIntegerFromStack(int index, bool noError)
	{
		if (IsInteger(index))
		{
			int number = (int)lua_tonumber(m_luaState, index);
			lua_remove(m_luaState, index);
			return number;
		}
		else if (!noError)
		{
			Error("The value on the stack is not an number");
		}
		return 0;
	}

	//Checks if the current index on the stack is an float and return it
	float LuaW::GetFloatFromStack(int index, bool noError)
	{
		if (IsNumber(index))
		{
			float number = (float)lua_tonumber(m_luaState, index);
			lua_remove(m_luaState, index);
			return number;
		}
		else if (!noError)
		{
			Error("The value on the stack is not an number");
		}
		return 0.0f;
	}

	//Checks if the current index on the stack is an double and return it
	double LuaW::GetDoubleFromStack(int index, bool noError)
	{
		if (IsNumber(index))
		{
			double number = lua_tonumber(m_luaState, index);
			lua_remove(m_luaState, index);
			return number;
		}
		else if (!noError)
		{
			Error("The value on the stack is not an number");
		}
		return 0.0;
	}

	//Checks if the current index on the stack is an string and return it
	std::string LuaW::GetStringFromStack(int index, bool noError)
	{
		if (IsString(index))
		{
			std::string string = std::string(lua_tostring(m_luaState, index));
			lua_remove(m_luaState, index);
			return std::move(string);
		}
		else if (!noError)
		{
			Error("The value on the stack is not an number");
		}
		return std::string();
	}

	//Checks if the current index on the stack is an bool and return it
	bool LuaW::GetBoolFromStack(int index, bool noError)
	{
		if (IsBool(index))
		{
			bool boolean = lua_toboolean(m_luaState, index);
			lua_remove(m_luaState, index);
			return boolean;
		}
		else if (!noError)
		{
			Error("The value on the stack is not an boolean");
		}
		return false;
	}

	//Checks if the current index on the stack is an function and return it
	Function LuaW::GetFunctionFromStack(int index, bool noError)
	{
		if (IsFunction(index))
		{
			lua_pushvalue(m_luaState, index);
			Function function = { luaL_ref(m_luaState, LUA_REGISTRYINDEX) };
			AddReference(function);
			lua_remove(m_luaState, index);
			return function;
		}
		else if (!noError)
		{
			Error("The value on the stack is not an function");
		}
		return Function(c_unValid);
	}

	//Checks if the current index on the stack is an table and return it
	Table LuaW::GetTableFromStack(int index, bool noError)
	{
		if (IsTable(index))
		{
			lua_pushvalue(m_luaState, index);
			Table table = { luaL_ref(m_luaState, LUA_REGISTRYINDEX) };
			AddReference(table);
			lua_remove(m_luaState, index);
			return table;
		}
		else if (!noError)
		{
			Error("The value on the stack is not an table");
		}
		lua_remove(m_luaState, index);
		return Table(c_unValid);
	}

	//Checks if the current index on the stack is an UserData and return it
	UserData LuaW::GetUserDataFromStack(int index, bool noError)
	{
		if (IsUserData(index))
		{
			lua_pushvalue(m_luaState, index);
			UserData userData = { luaL_ref(m_luaState, LUA_REGISTRYINDEX) };
			AddReference(userData);
			lua_remove(m_luaState, index);
			return userData;
		}
		else if (!noError)
		{
			Error("The value on the stack is not an userdata");
		}
		return UserData(c_unValid);
	}

	void LuaW::GetReturnsFromFunction(LuaFunctionReturn& luaFunctionReturn)
	{
		while (GetNumberOfStackItems() > 0)
		{
			if (IsInteger())
				luaFunctionReturn.integer = GetIntegerFromStack();
			else if (IsNumber())
				luaFunctionReturn.number = GetDoubleFromStack();
			else if (IsBool())
				luaFunctionReturn.boolean = GetBoolFromStack();
			else if (IsString())
				luaFunctionReturn.string = GetStringFromStack();
			else if (IsTable())
				luaFunctionReturn.table = GetTableFromStack();
			else if (IsFunction())
				luaFunctionReturn.function = GetFunctionFromStack();
			else if (IsUserData())
				luaFunctionReturn.userData = GetUserDataFromStack();
			else
				Error("Return is not an acceptable type");
		}
	}

	//Push an integer to the stack
	void LuaW::PushIntegerToStack(int integer)
	{
		lua_pushinteger(m_luaState, integer);
	}

	//Push an float to the stack
	void LuaW::PushFloatToStack(float number)
	{
		lua_pushnumber(m_luaState, number);
	}

	//Push an double to the stack
	void LuaW::PushDoubleToStack(double number)
	{
		lua_pushnumber(m_luaState, number);
	}

	//Push an string to the stack
	void LuaW::PushStringToStack(const std::string& string)
	{
		lua_pushstring(m_luaState, string.c_str());
	}

	//Push an string to the stack
	void LuaW::PushStringToStack(const char* string)
	{
		lua_pushstring(m_luaState, string);
	}

	//Push an bool to the stack
	void LuaW::PushBoolToStack(bool boolean)
	{
		lua_pushboolean(m_luaState, boolean);
	}

	//Push the function reference to the stack
	void LuaW::PushFunctionToStack(Function& function)
	{
		lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, function.ref);
	}

	//Push the table reference to the stack
	void LuaW::PushTableToStack(Table& table)
	{
		lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, table.ref);
	}

	//Push the UserData reference to the stack
	void LuaW::PushUserDataToStack(UserData& userData)
	{
		lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, userData.ref);
	}

	//Takes the top of the stack and pushes it to lua with the name
	void LuaW::SetGlobal(const std::string& luaGlobalName)
	{
		lua_setglobal(m_luaState, luaGlobalName.c_str());
	}

	//Takes the global from lua and pushes it to the stack
	void LuaW::GetGlobal(const std::string& luaGlobalName)
	{
		lua_getglobal(m_luaState, luaGlobalName.c_str());
	}

	//Pushes two items on the stack into the table and then we pop the table from the stack
	void LuaW::SetTableAndPop()
	{
		const int tableStackIndex = -3;
		const int popAmount = 1;

		lua_settable(m_luaState, tableStackIndex);

		if (GetNumberOfStackItems() > 0)
			lua_pop(m_luaState, popAmount);
	}

	//Gets item from the table and then we remove it from the stack
	void LuaW::GetTableAndRemove()
	{
		const int tableStackIndex = -2;
		const int popAmount = 1;

		lua_gettable(m_luaState, tableStackIndex);

		if (GetNumberOfStackItems() > 0)
			lua_remove(m_luaState, popAmount);
	}

	void LuaW::AddReference(Table& ref)
	{
		const int oneReference = 1;

		auto it = m_referenceList.find(ref.ref);
		if (it == m_referenceList.end())
			m_referenceList.insert({ ref.ref, {oneReference} });
		else
			++it->second.nrOfReferences;
	}

	//If there are no more reference to that table then we will remove the reference on the c++ side
	void LuaW::RemoveReference(Table& ref)
	{
		const int minAllowedReferencesForRemoval = 0;

		auto it = m_referenceList.find(ref.ref);
		if (it != m_referenceList.end())
		{
			--it->second.nrOfReferences;
			if (it->second.nrOfReferences == minAllowedReferencesForRemoval)
			{
				luaL_unref(m_luaState, LUA_REGISTRYINDEX, ref.ref);
			}
			ref.ref = c_unValid;
		}
	}

	int LuaW::AddEnvironmentToTable(Table& table, const std::string& luaFileName)
	{
		const int environmentTable = 1;
		const int globalTableStackIndex = 3;
		const int chunkStackIndex = -2;

		//Pushes the table to the stack
		PushTableToStack(table);

		//Pushes the lua file to the stack
		bool loadedChunk = LoadChunk(luaFileName);

		if (!loadedChunk)
			return true;

		//Creates a new table on the stack which is used as an metatable
		lua_newtable(m_luaState);

		//Get the global table and pusehs to the stack (lua defined table)
		GetGlobal("_G");

		//Sets the global table to the __index of the metatable
		lua_setfield(m_luaState, globalTableStackIndex, "__index");

		//Sets the metatable to the table
		lua_setmetatable(m_luaState, environmentTable);

		//Copies the table to the stack on the top
		lua_pushvalue(m_luaState, environmentTable);

		//Pushes the table to the chunk
		lua_setupvalue(m_luaState, chunkStackIndex, environmentTable);

		//Run the chunk (run the lua file)
		return lua_pcall(m_luaState, 0, LUA_MULTRET, 0);
	}

	int LuaW::GetNumberOfStackItems() const
	{
		return lua_gettop(m_luaState);
	}

	void LuaW::ClearStack()
	{
		const int popAmount = -1;
		//const int popAmount = -1;
		//When popAmount is -1 then it will clear the entire stack
		lua_pop(m_luaState, popAmount);
	}

	LuaW& LuaW::GetLuaW()
	{
		return s_luaW;
	}

	void LuaW::CallLuaFunction(Function& function, int arguments)
	{
		const int getErrorMessage = -1;
		const int bottomOfStackIndex = 1;

		if (function.ref == c_unValid)
		{
			Error("Function references nothing");
			return;
		}
		//Pushes the function to the stack
		PushFunctionToStack(function);

		//Only move the function and table on the stack if there are actually arguments 
		if (arguments > 0)
		{
			//Insert the function to the bottom of the stack
			//The arguments need to be at the top and the function call needs to be last
			lua_insert(m_luaState, bottomOfStackIndex);
		}

		//Call the function with the amount of arguments
		int error = lua_pcall(m_luaState, arguments, LUA_MULTRET, 0);
		if (error)
		{
			Error(lua_tostring(m_luaState, getErrorMessage));
		}
	}

	LuaFunctionReturn LuaW::CallLuaFunctionReturn(Function& function, int arguments)
	{
		CallLuaFunction(function, arguments);

		LuaFunctionReturn returns;

		GetReturnsFromFunction(returns);

		return returns;
	}

	void LuaW::CallTableLuaFunction(Table& table, Function& function, int arguments)
	{
		const int getErrorMessage = -1;
		const int bottomOfStackIndex = 1;

		if (function.ref == c_unValid || table.ref == c_unValid)
		{
			Error("Table or Function references nothing");
			return;
		}

		//Pushes the function to the stack
		PushFunctionToStack(function);
		//Pushes the table to the stack
		PushTableToStack(table);

		//Only move the function and table on the stack if there are actually arguments 
		if (arguments > 0)
		{
			//Insert the table
			//If you want to be able to access private table variables in the function then this is needed
			//This is equivalent to writing table.function(table) or table:function()
			lua_insert(m_luaState, bottomOfStackIndex);
			//Insert the function to the bottom of the stack under the table
			//The arguments need to be at the top and the function call needs to be last
			lua_insert(m_luaState, bottomOfStackIndex);

		}

		//+1 because we need to return the table aswell
		int argumentSize = 1 + arguments;

		int error = lua_pcall(m_luaState, argumentSize, LUA_MULTRET, 0);
		if (error)
		{
			Error(lua_tostring(m_luaState, getErrorMessage));
		}
	}

	LuaFunctionReturn LuaW::CallTableLuaFunctionReturn(Table& table, Function& function, int arguments)
	{
		CallTableLuaFunction(table, function, arguments);

		LuaFunctionReturn returns;

		GetReturnsFromFunction(returns);

		return returns;
	}

	Table LuaW::CreateGlobalTable(const std::string& globalTableName)
	{
		const int tableStackIndex = -1;

		//Creates a table on the stack
		lua_newtable(m_luaState);

		//Copies the table on the stack
		lua_pushvalue(m_luaState, tableStackIndex);

		//Take the copied table and pushes it as a global
		SetGlobal(globalTableName);

		//Then take the first table and create a refernce to the global table
		int index = luaL_ref(m_luaState, LUA_REGISTRYINDEX);

		Table table = { index };

		AddReference(table);

		return table;
	}

	Table LuaW::CreateTable()
	{
		//Creates a table on the stack
		lua_newtable(m_luaState);

		//Creates a reference to the table on the stack
		int index = luaL_ref(m_luaState, LUA_REGISTRYINDEX);

		Table table = { index };

		AddReference(table);

		return table;
	}

	Function LuaW::TryGetFunctionFromTable(Table& table, const std::string& tableFunctionName)
	{
		const int popAmount = 1;

		PushTableToStack(table);
		PushStringToStack(tableFunctionName);

		//Get the function from the table and remove the table from the stack
		GetTableAndRemove();

		//Get top
		int index = 1;

		Function function = GetFunctionFromStack(index, true);

		if (!CheckIfFunctionExist(function) && GetNumberOfStackItems() > 0)
			lua_pop(m_luaState, popAmount);

		return function;
	}

	//Checks if the function differs from unValid
	bool LuaW::CheckIfFunctionExist(Function& function)
	{
		return function.ref != c_unValid;
	}

	//Load the lua file and pushes it to the stack
	bool LuaW::LoadChunk(const std::string& luaScriptName)
	{
		const int getErrorMessage = -1;

		int error = luaL_loadfile(m_luaState, luaScriptName.c_str());
		if (error)
		{
			Error(lua_tostring(m_luaState, getErrorMessage));//"Couldn't find file " + luaScriptName + "!");
			return false;
		}

		return true;
	}

	//Try and get the lua file
	bool LuaW::TryLoadChunk(const std::string& luaScriptName)
	{
		const int popAmount = 1;
		const int getErrorMessage = -1;

		int error = luaL_loadfile(m_luaState, luaScriptName.c_str());
		if (error)
			Error(lua_tostring(m_luaState, getErrorMessage));
		else
			lua_pop(m_luaState, popAmount);
		return error;
	}

	void LuaW::PushStack(int integer)
	{
		PushIntegerToStack(integer);
	}

	void LuaW::PushStack(unsigned int integer)
	{
		PushIntegerToStack(integer);
	}

	void LuaW::PushStack(float number)
	{
		PushFloatToStack(number);
	}

	void LuaW::PushStack(double number)
	{
		PushDoubleToStack(number);
	}

	void LuaW::PushStack(bool boolean)
	{
		PushBoolToStack(boolean);
	}

	void LuaW::PushStack(const std::string& string)
	{
		PushStringToStack(string);
	}

	void LuaW::PushStack(const char* string)
	{
		PushStringToStack(string);
	}

	void LuaW::PushStack(Table& table)
	{
		PushTableToStack(table);
	}

	void LuaW::PushStack(LuaTable& table)
	{
		PushTableToStack(table.GetTable());
	}

	void LuaW::PushStack()
	{
		//Do nothing
	}

	//Checks if the index on the stack is a integer
	bool LuaW::IsInteger(int index) const
	{
		return lua_isinteger(m_luaState, index);
	}

	//Checks if the index on the stack is a number
	bool LuaW::IsNumber(int index) const
	{
		return lua_isnumber(m_luaState, index);
	}

	//Checks if the index on the stack is a string
	bool LuaW::IsString(int index) const
	{
		return lua_isstring(m_luaState, index);
	}

	//Checks if the index on the stack is a bool
	bool LuaW::IsBool(int index) const
	{
		return lua_isboolean(m_luaState, index);
	}

	//Checks if the index on the stack is a function
	bool LuaW::IsFunction(int index) const
	{
		return lua_isfunction(m_luaState, index);
	}

	//Checks if the index on the stack is a table
	bool LuaW::IsTable(int index) const
	{
		return lua_istable(m_luaState, index);
	}

	//Checks if the index on the stack is a userData
	bool LuaW::IsUserData(int index) const
	{
		return lua_isuserdata(m_luaState, index);
	}

	void LuaW::CreateEnvironment(Table& table, const std::string& luaFileName)
	{
		const int popAmount = 1;
		const int getErrorMessage = -1;

		int error = AddEnvironmentToTable(table, luaFileName);

		if (error)
		{
			if (GetNumberOfStackItems() > 0)
			{
				auto errorText = lua_tostring(m_luaState, getErrorMessage);
				Error(errorText);
			}
		}
		else
		{
			//Pop the table
			lua_pop(m_luaState, popAmount);
		}
	}

	int LuaW::TryCreateEnvironment(Table& table, const std::string& luaFileName)
	{
		const int popAmount = 1;
		const int getErrorMessage = -1;

		int error = AddEnvironmentToTable(table, luaFileName);

		if (error)
		{
			auto errorText = lua_tostring(m_luaState, getErrorMessage);
			Error(errorText);
		}
		else
		{
			//Pop the table
			lua_pop(m_luaState, popAmount);
		}

		return error;
	}

	//Adds reference to the table
	void LuaW::AddReferenceToTable(Table& table)
	{
		AddReference(table);
	}

	//Adds reference to the function
	void LuaW::AddReferenceToFunction(Function& function)
	{
		AddReference(function);
	}

	//Adds reference to the userData
	void LuaW::AddReferenceToUserData(UserData& userData)
	{
		AddReference(userData);
	}

	//Removes the reference to the table
	void LuaW::RemoveReferenceToTable(Table& table)
	{
		RemoveReference(table);
	}

	//Removes the reference to the function
	void LuaW::RemoveReferenceToFunction(Function& function)
	{
		RemoveReference(function);
	}

	//Removes the reference to the UserData
	void LuaW::RemoveReferenceToUserData(UserData& userData)
	{
		RemoveReference(userData);
	}
}
