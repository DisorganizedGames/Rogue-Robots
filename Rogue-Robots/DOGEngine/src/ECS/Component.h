#pragma once
namespace DOG
{
	struct ComponentBase
	{
		ComponentBase() noexcept = default;
	protected:
		[[nodiscard]] static const u32 GetID() noexcept;
	};

	class EntityManager;
	template<typename T>
	struct Component : public ComponentBase
	{
		friend EntityManager;
		Component() noexcept = default;
	private:
		static const u32 ID;
	};

	template<typename T>
	const u32 Component<T>::ID{ ComponentBase::GetID() };

	struct TransformComponent : public Component<TransformComponent>
	{
		TransformComponent(Vector3f position = { 0.0f, 0.0f, 0.0f }) noexcept : m_position{ position } {}
		Vector3f m_position;
	};

	struct ModelComponent : public Component<ModelComponent>
	{
		ModelComponent(u64 id = 0) noexcept : id{ id } {}
		u64 id;
	};
}