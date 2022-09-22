#pragma once
#include "LuaTable.h"

namespace DOG
{
	class LuaW;

	class LuaContext
	{
	private:
		LuaW* m_luaW;
		uint32_t m_returnArgumentSize;

	private:
		void IncreaseReturnArgument();

	public:
		LuaContext(LuaW* luaW);

		int GetInteger() const;
		double GetDouble() const;
		bool GetBoolean() const;
		std::string GetString() const;
		LuaTable GetTable() const;
		template <typename T>
		T* GetUserData() const;

		void ReturnInteger(int integer);
		void ReturnDouble(double number);
		void ReturnBoolean(bool boolean);
		void ReturnString(const std::string& string);
		void ReturnTable(LuaTable& luaTable);
		template <typename T>
		void ReturnUserData(T* userData, const std::string& interfaceName);

		uint32_t GetNumberOfReturns() const;
	};

	template<typename T>
	inline T* LuaContext::GetUserData() const
	{
		int max = m_luaW->GetNumberOfStackItems();

		for (int index = 1; index <= max; ++index)
			if (m_luaW->IsUserData(index))
			{
				return m_luaW->GetUserDataPointerFromStack<T>(index);
			}

		std::cout << "Error: Couldn't Find Table In Arguments\n";

		return nullptr;
	}

	template<typename T>
	inline void LuaContext::ReturnUserData(T* userData, const std::string& interfaceName)
	{
		IncreaseReturnArgument();
		m_luaW->PushUserDataPointerToStack<T>(userData, interfaceName);
	}
}