#pragma once
#include <typeindex>
#include <unordered_map>
#include <memory>

namespace DOG::gfx
{
	class RGBlackboard
	{
	public:
		template <typename T>
		void Add(const T& data)
		{
			assert(!m_datas.contains(typeid(T)));

			auto& storage = m_datas[typeid(T)];
			storage = std::make_unique<uint8_t[]>(sizeof(T));
			std::memcpy(storage.get(), &data, sizeof(T));
		}

		template <typename T>
		void Remove()
		{
			m_datas.erase(typeid(T));
		}

		template <typename T>
		const T& Get()
		{
			return *(const T*)(m_datas[typeid(T)].get());
		}

	private:
		std::unordered_map<std::type_index, std::unique_ptr<uint8_t[]>> m_datas;
	};
}