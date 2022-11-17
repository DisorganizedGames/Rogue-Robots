#pragma once

extern "C"
{
#include "Lua/lua.h"
#include "Lua/lauxlib.h"
#include "Lua/lualib.h"
}

namespace DOG
{
	class LuaContext;
	class LuaGlobal;
	class LuaTable;
	class LuaEvent;
	class LuaMain;

	struct ClassFunctionInfo
	{
		std::string functionName;
		lua_CFunction classFunction = {0};
	};

	struct RegisterClassFunctions
	{
		std::string className;
		std::vector<ClassFunctionInfo> classFunctions;

		template <typename T, void (T::* func)(LuaContext*)>
		RegisterClassFunctions AddFunction(const std::string& functionName);
		template <void (*func)(LuaContext*)>
		RegisterClassFunctions AddStaticFunction(const std::string& functionName);
	};

	struct Table
	{
		int ref;
	};

	typedef Table Function;
	typedef Table UserData;
	typedef Table Coroutine;

	struct LuaFunctionReturn
	{
		int integer = 0;
		double number = 0.0;
		bool boolean = false;
		std::string string = "";
		Table table = { -1 };
		Function function = { -1 };
		UserData userData = { -1 };
	};

	struct Reference
	{
		uint32_t nrOfReferences;
	};

	class LuaW
	{
		friend RegisterClassFunctions;
		friend LuaContext;
		friend LuaGlobal;
		friend LuaTable;
		friend LuaEvent;
		friend LuaMain;

	private:
		lua_State* m_luaState;
		std::unordered_map<int, Reference> m_referenceList;
		static constexpr int c_unValid = -1;
		static LuaW s_luaW;

	private:
		LuaW(lua_State* l);
		template<void (*func)(LuaContext*)>
		static inline int FunctionsHook(lua_State* luaState);

		template <typename T, void (T::* func)(LuaContext*)>
		static inline int ClassFunctionsHook(lua_State* luaState);

		template<void (*func)(LuaContext*)>
		Function HookFunctionAndGetFunction();

		void Error(const std::string& errorMessage);

		bool LoadChunk(const std::string& luaScriptName);

		//Push arguments to stack
		template<typename T, class... Args>
		void PushStack(T type, Args&&... args);

		//Push integer
		void PushStack(int integer);
		//Push unsigned integer
		void PushStack(unsigned int integer);
		//Push float
		void PushStack(float number);
		//Push double
		void PushStack(double number);
		//Push bool
		void PushStack(bool boolean);
		//Push string
		void PushStack(const std::string& string);
		//Push string
		void PushStack(const char* string);
		//Push table, Used for UserData and functions aswell
		void PushStack(Table& table);
		//Push LuaTable
		void PushStack(LuaTable& table);
		//Does nothing (Exist because it is needed for Args... to push an empty argument)
		void PushStack();

		bool IsInteger(int index = 1) const;
		bool IsNumber(int index = 1) const;
		bool IsString(int index = 1) const;
		bool IsBool(int index = 1) const;
		bool IsFunction(int index = 1) const;
		bool IsTable(int index = 1) const;
		bool IsUserData(int index = 1) const;
		bool IsThread(int index = 1) const;

		template<typename T>
		bool TryGetValueFromStack(T& valueOut)
		{
			return false;
		}

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
		lua_State* GetThreadPointerFromStack(int index = 1);

		void GetReturnsFromFunction(LuaFunctionReturn& luaFunctionReturn);

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
		void PushThreadToStack(Coroutine& coroutine);

		void SetGlobal(const std::string& luaGlobalName);
		void GetGlobal(const std::string& luaGlobalName);
		void SetTableAndPop();
		void GetTableAndRemove();

		void AddReference(Table& ref);
		void RemoveReference(Table& ref);

		int AddEnvironmentToTable(Table& table, const std::string& luaFileName);

		int GetNumberOfStackItems() const;
		int GetNumberOfStackItems(lua_State* thread) const;

		void ClearStack();

		//Move function from one thread to another
		static void MoveStackItemsBetweenThreads(lua_State* source, lua_State* destination, int amount);

		static LuaW& GetLuaW();

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

		static void PrintStack();
		static void PrintStack(Coroutine& coroutine);
		static void PrintStack(lua_State* thread);

		int GetGlobalInteger(const std::string& luaGlobalName);
		float GetGlobalFloat(const std::string& luaGlobalName);
		double GetGlobalDouble(const std::string& luaGlobalName);
		std::string GetGlobalString(const std::string& luaGlobalName);
		bool GetGlobalBool(const std::string& luaGlobalName);
		Function GetGlobalFunction(const std::string& luaGlobalName);
		Table GetGlobalTable(const std::string& luaGlobalName);
		UserData GetGlobalUserData(const std::string& luaGlobalName);

		void CallLuaFunction(Function& function, int arguments = 0);
		LuaFunctionReturn CallLuaFunctionReturn(Function& function, int arguments = 0);
		void CallTableLuaFunction(Table& table, Function& function, int arguments = 0);
		LuaFunctionReturn CallTableLuaFunctionReturn(Table& table, Function& function, int arguments = 0);

		Table CreateTable();

		template<typename T>
		void AddNumberToTable(Table& table, const T& tableKey, int number);
		template<typename T>
		void AddNumberToTable(Table& table, const T& tableKey, float number);
		template<typename T>
		void AddNumberToTable(Table& table, const T& tableKey, double number);
		template<typename T>
		void AddStringToTable(Table& table, const T& tableKey, const std::string& string);
		template<typename T>
		void AddStringToTable(Table& table, const T& tableKey, const char* string);
		template<typename T>
		void AddBoolToTable(Table& table, const T& tableKey, bool boolean);
		template<typename T>
		void AddTableToTable(Table& table, const T& tableKey, Table& addTable);
		template<void (*func)(LuaContext*), typename T>
		void AddFunctionToTable(Table& table, const T& tableKey);
		template<typename T>
		void AddFunctionToTable(Table& table, const T& tableKey, Function& function);
		template<typename UserDataType, typename T>
		void AddUserDataToTable(Table& table, UserDataType* object, const std::string& interfaceName, const T& tableKey);
		template<typename T>
		void AddUserDataToTable(Table& table, const T& tableKey, UserData& userData);

		template<typename T>
		int GetIntegerFromTable(Table& table, const T& tableGet);
		template<typename T>
		bool TryGetIntegerFromTable(Table& table, const T& tableGet, int& out);
		template<typename T>
		float GetFloatFromTable(Table& table, const T& tableGet);
		template<typename T>
		bool TryGetFloatFromTable(Table& table, const T& tableGet, float& out);
		template<typename T>
		double GetDoubleFromTable(Table& table, const T& tableGet);
		template<typename T>
		bool TryGetDoubleFromTable(Table& table, const T& tableGet, double& out);
		template<typename T>
		std::string GetStringFromTable(Table& table, const T& tableGet);
		template<typename T>
		bool TryGetStringFromTable(Table& table, const T& tableGet, std::string& out);
		template<typename T>
		bool GetBoolFromTable(Table& table, const T& tableGet);
		template<typename T>
		bool TryGetBoolFromTable(Table& table, const T& tableGet, bool& out);
		template<typename T>
		Function GetFunctionFromTable(Table& table, const T& tableGet);
		template<typename T>
		Table GetTableFromTable(Table& table, const T& tableGet);
		template<typename T>
		UserData GetUserDataFromTable(Table& table, const T& tableGet);

		Function TryGetFunctionFromTable(Table& table, const std::string& tableFunctionName);
		bool CheckIfFunctionExist(Function& function);

		Coroutine CreateThread();
		void CreateCoroutine(Coroutine& coroutine, Function& function);
		bool CoroutineIsDead(Coroutine& coroutine);
		void ResumeCoroutine(Coroutine& coroutine);

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
		void RemoveReferenceToCoroutine(Coroutine& coroutine);
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
		T* userDataPointer = *(T**)lua_touserdata(m_luaState, index);
		lua_remove(m_luaState, index);
		return userDataPointer;
	}

	//Pushes a new UserData to the stack
	template<typename T>
	inline void LuaW::PushUserDataPointerToStack(T* object, const std::string& interfaceName)
	{
		T** newUserData = (T**)lua_newuserdata(m_luaState, sizeof(T*));
		*newUserData = object;

		//Gets the metatable of the interface
		luaL_getmetatable(m_luaState, interfaceName.c_str());
		const int userDataPosition = -2;
		//Sets the metatable of the interface to the userData
		lua_setmetatable(m_luaState, userDataPosition);
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
	static inline int LuaW::FunctionsHook(lua_State* thread)
	{
		LuaContext state(&s_luaW);

		bool isACoroutine = thread != s_luaW.m_luaState;
		//Move all the function call information to the main thread (m_luaState)
		if (isACoroutine)
		{
			//Move function from one thread to another (from the coroutine to the main lua_State)
			MoveStackItemsBetweenThreads(thread, s_luaW.m_luaState, s_luaW.GetNumberOfStackItems(thread));
		}

		func(&state);

		//Move all the returns from the main lua state to the coroutine
		if (isACoroutine)
		{
			//Move function from one thread to another (from the main lua_State to the coroutine)
			MoveStackItemsBetweenThreads(s_luaW.m_luaState, thread, state.GetNumberOfReturns());
		}

		return state.GetNumberOfReturns();
	}

	//Hooks a member function from c++ to an static int function(lua_State* luaState) 
	template <typename T, void (T::* func)(LuaContext*)>
	static inline int LuaW::ClassFunctionsHook(lua_State* thread)
	{
		LuaContext state(&s_luaW);

		bool isACoroutine = thread != s_luaW.m_luaState;
		//Move all the function call information to the main thread (m_luaState)
		if (isACoroutine)
		{
			//Move function from one thread to another (from the coroutine to the main lua_State)
			MoveStackItemsBetweenThreads(thread, s_luaW.m_luaState, s_luaW.GetNumberOfStackItems(thread));
		}

		if (!s_luaW.IsUserData())
		{
			s_luaW.Error("Tried to access userdata and no userdata was found! Call object function with either object:function() or object.function(object)!");
			return 0;
		}

		T* object = s_luaW.GetUserDataPointerFromStack<T>();
		(object->*func)(&state);

		//Move all the returns from the main lua state to the coroutine
		if (isACoroutine)
		{
			//Move function from one thread to another (from the main lua_State to the coroutine)
			MoveStackItemsBetweenThreads(s_luaW.m_luaState, thread, state.GetNumberOfReturns());
		}

		return state.GetNumberOfReturns();
	}

	template<void(*func)(LuaContext*)>
	inline Function LuaW::HookFunctionAndGetFunction()
	{
		lua_pushcfunction(m_luaState, FunctionsHook<func>);
		return GetFunctionFromStack();
	}

	template<typename T, class ...Args>
	inline void LuaW::PushStack(T type, Args&& ...args)
	{
		//Push type to the stack
		//The other push stack functions are called here!
		PushStack(type);

		//Continues the recursive function by taking out one more argument from the arg list
		//Calls the empty push stack when it is done
		PushStack(std::forward<Args>(args)...);
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

	template<void(*func)(LuaContext*)>
	inline RegisterClassFunctions RegisterClassFunctions::AddStaticFunction(const std::string& functionName)
	{
		ClassFunctionInfo info;
		info.functionName = functionName;
		info.classFunction = LuaW::FunctionsHook<func>;

		classFunctions.push_back(info);

		return *this;
	}

	template<typename T>
	inline int LuaW::GetIntegerFromTable(Table& table, const T& tableGet)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get integer
		PushStack(tableGet);

		//Get the integer from the table and remove the table from the stack
		GetTableAndRemove();

		return GetIntegerFromStack();
	}

	template<typename T>
	inline bool LuaW::TryGetIntegerFromTable(Table& table, const T& tableGet, int& out)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get integer
		PushStack(tableGet);

		//Get the integer from the table and remove the table from the stack
		GetTableAndRemove();

		if (IsInteger())
		{
			out = GetIntegerFromStack(1, true);
			return true;
		}
		else
		{
			ClearStack();
			return false;
		}
	}

	template<typename T>
	inline float LuaW::GetFloatFromTable(Table& table, const T& tableGet)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get float
		PushStack(tableGet);

		//Get the float from the table and remove the table from the stack
		GetTableAndRemove();

		return GetFloatFromStack();
	}

	template<typename T>
	inline bool LuaW::TryGetFloatFromTable(Table& table, const T& tableGet, float& out)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get float
		PushStack(tableGet);

		//Get the float from the table and remove the table from the stack
		GetTableAndRemove();

		if (IsNumber())
		{
			out = GetFloatFromStack(1, true);
			return true;
		}
		else
		{
			ClearStack();
			return false;
		}
	}

	template<typename T>
	inline double LuaW::GetDoubleFromTable(Table& table, const T& tableGet)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get double
		PushStack(tableGet);

		//Get the double from the table and remove the table from the stack
		GetTableAndRemove();

		return GetDoubleFromStack();
	}

	template<typename T>
	inline bool LuaW::TryGetDoubleFromTable(Table& table, const T& tableGet, double& out)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get double
		PushStack(tableGet);

		//Get the double from the table and remove the table from the stack
		GetTableAndRemove();

		if (IsNumber())
		{
			out = GetDoubleFromStack(1, true);
			return true;
		}
		else
		{
			ClearStack();
			return false;
		}
	}

	template<typename T>
	inline std::string LuaW::GetStringFromTable(Table& table, const T& tableGet)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get string
		PushStack(tableGet);

		//Get the string from the table and remove the table from the stack
		GetTableAndRemove();

		return GetStringFromStack();
	}

	template<typename T>
	inline bool LuaW::TryGetStringFromTable(Table& table, const T& tableGet, std::string& out)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get string
		PushStack(tableGet);

		//Get the string from the table and remove the table from the stack
		GetTableAndRemove();

		if (IsString())
		{
			out = std::move(GetStringFromStack(1, true));
			return true;
		}
		else
		{
			ClearStack();
			return false;
		}
	}

	template<typename T>
	inline bool LuaW::GetBoolFromTable(Table& table, const T& tableGet)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get bool
		PushStack(tableGet);

		//Get the bool from the table and remove the table from the stack
		GetTableAndRemove();

		return GetBoolFromStack();
	}

	template<typename T>
	inline bool LuaW::TryGetBoolFromTable(Table& table, const T& tableGet, bool& out)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get bool
		PushStack(tableGet);

		//Get the bool from the table and remove the table from the stack
		GetTableAndRemove();

		if (IsBool())
		{
			out = GetBoolFromStack(1, true);
			return true;
		}
		else
		{
			ClearStack();
			return false;
		}
	}

	template<typename T>
	inline Function LuaW::GetFunctionFromTable(Table& table, const T& tableGet)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get function
		PushStack(tableGet);

		//Get the function from the table and remove the table from the stack
		GetTableAndRemove();

		return GetFunctionFromStack();
	}

	template<typename T>
	inline Table LuaW::GetTableFromTable(Table& table, const T& tableGet)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get table
		PushStack(tableGet);

		//Get the table from the table and remove the table from the stack
		GetTableAndRemove();

		return GetTableFromStack();
	}

	template<typename T>
	inline UserData LuaW::GetUserDataFromTable(Table& table, const T& tableGet)
	{
		//Pushes the table to the stack
		PushTableToStack(table);
		//Pushes int/string to get UserData
		PushStack(tableGet);

		//Get the UserData from the table and remove the table from the stack
		GetTableAndRemove();

		return GetUserDataFromStack();
	}

	//Gets the UserData from the stack
	template<typename T>
	inline T* LuaW::GetUserDataPointer(UserData& userData)
	{
		PushUserDataToStack(userData);
		return GetUserDataPointerFromStack<T>();
	}

	template<typename T>
	inline void LuaW::AddNumberToTable(Table& table, const T& tableKey, int number)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the number to the stack
		PushIntegerToStack(number);

		//Sets the number to the tableKey in the table
		SetTableAndPop();
	}

	template<typename T>
	inline void LuaW::AddNumberToTable(Table& table, const T& tableKey, float number)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the number to the stack
		PushFloatToStack(number);

		//Sets the number to the tableKey in the table
		SetTableAndPop();
	}

	template<typename T>
	inline void LuaW::AddNumberToTable(Table& table, const T& tableKey, double number)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the number to the stack
		PushDoubleToStack(number);

		//Sets the number to the tableKey in the table
		SetTableAndPop();
	}

	template<typename T>
	inline void LuaW::AddStringToTable(Table& table, const T& tableKey, const std::string& string)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the string to the stack
		PushStringToStack(string);

		//Sets the string to the tableKey in the table
		SetTableAndPop();
	}

	template<typename T>
	inline void LuaW::AddStringToTable(Table& table, const T& tableKey, const char* string)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the string to the stack
		PushStringToStack(string);

		//Sets the string to the tableKey in the table
		SetTableAndPop();
	}

	template<typename T>
	inline void LuaW::AddBoolToTable(Table& table, const T& tableKey, bool boolean)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the bool to the stack
		PushBoolToStack(boolean);

		//Sets the bool to the tableKey in the table
		SetTableAndPop();
	}

	template<typename T>
	inline void LuaW::AddTableToTable(Table& table, const T& tableKey, Table& addTable)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the table to the stack
		PushTableToStack(addTable);

		//Sets the table to the tableKey in the table
		SetTableAndPop();
	}

	template<void(*func)(LuaContext*), typename T>
	inline void LuaW::AddFunctionToTable(Table& table, const T& tableKey)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the function to the stack
		lua_pushcfunction(m_luaState, FunctionsHook<func>);

		//Sets the function to the tableKey in the table
		SetTableAndPop();
	}

	template<typename T>
	inline void LuaW::AddFunctionToTable(Table& table, const T& tableKey, Function& function)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the function to the stack
		PushFunctionToStack(function);

		//Sets the function to the tableKey in the table
		SetTableAndPop();
	}

	template<typename UserDataType, typename T>
	inline void LuaW::AddUserDataToTable(Table& table, UserDataType* object, const std::string& interfaceName, const T& tableKey)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the UserData to the stack
		PushUserDataPointerToStack<UserDataType>(object, interfaceName);

		//Sets the UserData to the tableKey in the table
		SetTableAndPop();
	}

	template<typename T>
	inline void LuaW::AddUserDataToTable(Table& table, const T& tableKey, UserData& userData)
	{
		//Pushes the table on the stack
		PushTableToStack(table);
		//Pushes int/string on the stack
		PushStack(tableKey);
		//Pushes the UserData to the stack
		PushUserDataToStack(userData);

		//Sets the UserData to the tableKey in the table
		SetTableAndPop();
	}
}
