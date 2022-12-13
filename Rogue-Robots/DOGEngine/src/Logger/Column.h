#pragma once

namespace DOG
{
	class Column
	{
	public:
		std::string& operator[](size_t row) { return m_values[row]; };

		using str = std::string;
		void Add(str value) { m_values.push_back(value); }
		void Add(u64 value) { m_values.push_back(std::to_string(value)); }
		void Add(i64 value) { m_values.push_back(std::to_string(value)); }
		void Add(u32 value) { m_values.push_back(std::to_string(value)); }
		void Add(i32 value) { m_values.push_back(std::to_string(value)); }
		void Add(u8 value) { m_values.push_back(std::to_string(static_cast<u32>(value))); }
		void Add(i8 value) { m_values.push_back(std::to_string(static_cast<i32>(value))); }
		void Add(f64 value) { m_values.push_back(std::to_string(value)); }
		void Add(f32 value) { m_values.push_back(std::to_string(value)); }
		void Add(bool value) { m_values.push_back(std::to_string(value)); }
		
		size_t size() { return m_values.size(); }

		Column() = default;
		~Column() = default;

	private:
		std::vector<std::string> m_values;
	};
}