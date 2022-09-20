#pragma once
namespace DOG
{
	struct ComponentBase
	{
		ComponentBase() noexcept = default;
		virtual ~ComponentBase() noexcept = default;
		[[nodiscard]] static const u32 GetID() noexcept;
	};

	template<typename T>
	struct Component : public ComponentBase
	{
		Component() noexcept = default;
		virtual ~Component() noexcept override = default;
		static u32 ID;
	};

	template<typename T>
	u32 Component<T>::ID{ ComponentBase::GetID() };

	struct TransformComponent : public Component<TransformComponent>
	{
		TransformComponent(Vector3f position = { 0.0f, 0.0f, 0.0f }) noexcept : position{ position } {}
		virtual ~TransformComponent() noexcept override final = default;
		Vector3f position;
	};

	struct MeshComponent : public Component<MeshComponent>
	{
		MeshComponent(u32 id = 0) noexcept : id{ id } {}
		virtual ~MeshComponent() noexcept override final = default;
		u32 id;
	};

	struct SoundComponent : public Component<SoundComponent>
	{
		SoundComponent(u32 id = 0) noexcept : id{ id } {}
		virtual ~SoundComponent() noexcept override final = default;
		u32 id;
	};
}