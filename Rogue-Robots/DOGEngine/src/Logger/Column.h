#pragma once

namespace DOG
{
	class Column
	{
	public:
		std::string& operator[](size_t row) { return m_values[row]; };

		void SetLastValue(std::string	value) { m_values.back() = value; }
		void SetLastValue(u64			value) { m_values.back() = std::to_string(value); }
		void SetLastValue(i64			value) { m_values.back() = std::to_string(value); }
		void SetLastValue(u32			value) { m_values.back() = std::to_string(value); }
		void SetLastValue(i32			value) { m_values.back() = std::to_string(value); }
		void SetLastValue(u8			value) { m_values.back() = std::to_string(static_cast<u32>(value)); }
		void SetLastValue(i8			value) { m_values.back() = std::to_string(static_cast<i32>(value)); }
		void SetLastValue(f64			value) { m_values.back() = std::to_string(value); }
		void SetLastValue(f32			value) { m_values.back() = std::to_string(value); }
		
		void NewRow() { m_values.push_back(""); }
		std::string& Header() { return m_header; }

		Column() = delete;
		Column(std::string header) : m_header(header) {}
		~Column() = default;

	private:
		std::string m_header;
		std::vector<std::string> m_values;
	};
}