#pragma once

extern "C"
{
#include "Lua/lua.h"
#include "Lua/lauxlib.h"
#include "Lua/lualib.h"
}

class LuaContext;

struct ClassFunctionInfo
{
	std::string functionName;
	lua_CFunction classFunction;
};

struct RegisterClassFunctions
{
	std::string className;
	std::vector<ClassFunctionInfo> classFunctions;

	template <typename T, void (T::* func)(LuaContext*)>
	RegisterClassFunctions AddFunction(const std::string& functionName);
};

struct Table
{
	int ref;
};

typedef Table Function;
typedef Table UserData;

struct Reference
{
	uint32_t nr_of_references;
};

class LuaW
{
	friend RegisterClassFunctions;
	friend LuaContext;

private:
	lua_State* m_luaState;
	std::unordered_map<int, Reference> m_reference_list;

private:
	LuaW(lua_State* l);
	template<void (*func)(LuaContext*)>
	static inline int FunctionsHook(lua_State* luaState);

	template <typename T, void (T::* func)(LuaContext*)>
	static inline int ClassFunctionsHook(lua_State* luaState);

	void Error(const std::string& errorMessage);

	void LoadChunk(const std::string& luaScriptName);

	bool IsInteger(int index = 1) const;
	bool IsNumber(int index = 1) const;
	bool IsString(int index = 1) const;
	bool IsBool(int index = 1) const;
	bool IsFunction(int index = 1) const;
	bool IsTable(int index = 1) const;
	bool IsUserData(int index = 1) const;

	int GetIntegerFromStack(int index = 1, bool noError = false);
	float GetFloatFromStack(int index = 1, bool noError = false);
	double GetDoubleFromStack(int index = 1, bool noError = false);
	std::string GetStringFromStack(int index = 1, bool noError = false);
	bool GetBoolFromStack(int index = 1, bool noError = false);
	Function GetFunctionFromStack(int index = 1, bool noError = false);
	Table GetTableFromStack(int index = 1, bool noError = false);
	UserData GetUserDataFromStack(int index = 1, bool noError = false);
	template<typename T>
	T* GetUserDataPointerFromStack(int index = 1);

	void PushIntegerToStack(int integer);
	void PushFloatToStack(float number);
	void PushDoubleToStack(double number);
	void PushStringToStack(const std::string& string);
	void PushStringToStack(const char* string);
	void PushBoolToStack(bool boolean);
	void PushFunctionToStack(Function& function);
	void PushTableToStack(Table& table);
	template<typename T>
	void PushUserDataPointerToStack(T* object, const std::string& interfaceName);
	void PushUserDataToStack(UserData& userData);

	void SetGlobal(const std::string& luaGlobalName);
	void GetGlobal(const std::string& luaGlobalName);
	void SetTableAndPop();
	void GetTableAndRemove();

	void AddReference(Table& ref);
	void RemoveReference(Table& ref);

	int AddEnvironmentToTable(Table& table, const std::string& luaFileName);

	int GetNumberOfStackItems() const;

	void ClearStack();

public:
	void PushGlobalNumber(const std::string& luaGlobalName, int number);
	void PushGlobalNumber(const std::string& luaGlobalName, float number);
	void PushGlobalNumber(const std::string& luaGlobalName, double number);
	void PushGlobalString(const std::string& luaGlobalName, const std::string& string);
	void PushGlobalString(const std::string& luaGlobalName, const char* string);
	void PushGlobalBool(const std::string& luaGlobalName, bool boolean);
	template<void (*func)(LuaContext*)>
	void PushGlobalFunction(const std::string& functionName);
	template<typename T>
	void PushGlobalUserData(T* object, const std::string& interfaceName, const std::string& objectName);
	Table CreateGlobalTable(const std::string& globalTableName);

	RegisterClassFunctions CreateLuaInterface(const std::string& interfaceName);
	void PushLuaInterface(RegisterClassFunctions& registerInterface);

	void PrintStack();

	int GetGlobalInteger(const std::string& luaGlobalName);
	float GetGlobalFloat(const std::string& luaGlobalName);
	double GetGlobalDouble(const std::string& luaGlobalName);
	std::string GetGlobalString(const std::string& luaGlobalName);
	bool GetGlobalBool(const std::string& luaGlobalName);
	Function GetGlobalFunction(const std::string& luaGlobalName);
	Table GetGlobalTable(const std::string& luaGlobalName);
	UserData GetGlobalUserData(const std::string& luaGlobalName);

	void CallLuaFunction(Function& function, int arguments = 0);
	void CallTableLuaFunction(Table& table, Function& function, int arguments = 0);

	Table CreateTable();

	void AddNumberToTable(Table& table, const std::string& numberName, int number);
	void AddNumberToTable(Table& table, const std::string& numberName, float number);
	void AddNumberToTable(Table& table, const std::string& numberName, double number);
	void AddStringToTable(Table& table, const std::string& stringName, const std::string& string);
	void AddStringToTable(Table& table, const std::string& stringName, const char* string);
	void AddBoolToTable(Table& table, const std::string& boolName, bool boolean);
	void AddTableToTable(Table& table, const std::string& tableName, Table& addTable);
	template<void (*func)(LuaContext*)>
	void AddFunctionToTable(Table& table, const std::string& functionName);
	void AddFunctionToTable(Table& table, const std::string& functionName, Function& function);
	template<typename T>
	void AddUserDataToTable(Table& table, T* object, const std::string& interfaceName, const std::string& objectName);
	void AddUserDataToTable(Table& table, const std::string& userDataName, UserData& userData);

	int GetIntegerFromTable(Table& table, const std::string& tableIntegerName);
	float GetFloatFromTable(Table& table, const std::string& tableFloatName);
	double GetDoubleFromTable(Table& table, const std::string& tableDoubleName);
	std::string GetStringFromTable(Table& table, const std::string& tableStringName);
	bool GetBoolFromTable(Table& table, const std::string& tableBoolName);
	Function GetFunctionFromTable(Table& table, const std::string& tableFunctionName);
	Table GetTableFromTable(Table& table, const std::string& tableTableName);
	UserData GetUserDataFromTable(Table& table, const std::string& tableUserDataName);

	Function TryGetFunctionFromTable(Table& table, const std::string& tableFunctionName);
	bool CheckIfFunctionExist(Function& function);

	void RunScript(const std::string& luaFileName);
	void CreateEnvironment(Table& table, const std::string& luaFileName);
	int TryCreateEnvironment(Table& table, const std::string& luaFileName);

	bool TryLoadChunk(const std::string& luaScriptName);

	template<typename T>
	T* GetUserDataPointer(UserData& userData);

	void AddReferenceToTable(Table& table);
	void AddReferenceToFunction(Function& function);
	void AddReferenceToUserData(UserData& userData);
	void RemoveReferenceToTable(Table& table);
	void RemoveReferenceToFunction(Function& function);
	void RemoveReferenceToUserData(UserData& userData);

public:
	LuaW();

};

//Sets the cfunction to an global variable in lua
template<void (*func)(LuaContext*)>
void LuaW::PushGlobalFunction(const std::string& functionName)
{
	lua_pushcfunction(m_luaState, FunctionsHook<func>);
	SetGlobal(functionName);
}

//Returns the UserData on the stack
template<typename T>
T* LuaW::GetUserDataPointerFromStack(int index)
{
	return *(T**)lua_touserdata(m_luaState, index);
}

//Pushes a new UserData to the stack
template<typename T>
inline void LuaW::PushUserDataPointerToStack(T* object, const std::string& interfaceName)
{
	T** newUserData = (T**)lua_newuserdata(m_luaState, sizeof(T*));
	*newUserData = object;

	luaL_getmetatable(m_luaState, interfaceName.c_str());
	lua_setmetatable(m_luaState, -2);
}

//Sets the UserData to an global variable in lua
template<typename T>
void LuaW::PushGlobalUserData(T* object, const std::string& interfaceName, const std::string& objectName)
{
	PushUserDataPointerToStack<T>(object, interfaceName);

	SetGlobal(objectName);
}

//Hooks the function from c++ to an static int function(lua_State* luaState) 
template<void (*func)(LuaContext*)>
static inline int LuaW::FunctionsHook(lua_State* luaState)
{
	LuaW luaW(luaState);
	LuaContext state(&luaW);

	func(&state);

	return state.GetNumberOfReturns();
}

//Hooks a member function from c++ to an static int function(lua_State* luaState) 
template <typename T, void (T::* func)(LuaContext*)>
static inline int LuaW::ClassFunctionsHook(lua_State* luaState)
{
	LuaW luaW(luaState);
	LuaContext state(&luaW);

	if (!luaW.IsUserData())
	{
		luaW.Error("Tried to access userdata and no userdata was found! Call object function with either object:function() or object.function(object)!");
		return 0;
	}
	T* object = luaW.GetUserDataPointerFromStack<T>();
	(object->*func)(&state);

	return state.GetNumberOfReturns();
}

//Adds a member function to the interface
template <typename T, void (T::* func)(LuaContext*)>
RegisterClassFunctions RegisterClassFunctions::AddFunction(const std::string& functionName)
{
	ClassFunctionInfo info;
	info.functionName = functionName;
	info.classFunction = LuaW::ClassFunctionsHook<T, func>;

	classFunctions.push_back(info);

	return *this;
}

//Adds function from c++ to a table
template<void (*func)(LuaContext*)>
void LuaW::AddFunctionToTable(Table& table, const std::string& functionName)
{
	PushTableToStack(table);
	PushStringToStack(functionName);
	lua_pushcfunction(m_luaState, FunctionsHook<func>);

	SetTableAndPop();
}

//Adds UserData to a table 
template<typename T>
inline void LuaW::AddUserDataToTable(Table& table, T* object, const std::string& interfaceName, const std::string& objectName)
{
	PushTableToStack(table);
	PushStringToStack(objectName);
	PushUserDataPointerToStack<T>(object, interfaceName);

	SetTableAndPop();
}

//Gets the UserData from the stack
template<typename T>
inline T* LuaW::GetUserDataPointer(UserData& userData)
{
	PushUserDataToStack(userData);
	return GetUserDataPointerFromStack<T>();
}
