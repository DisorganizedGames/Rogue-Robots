#pragma once

namespace DOG
{
	class EntityManager;
	typedef u32 entity;

	enum class SceneType
	{
		Global = 0,
		MainScene,
		TestScene,
	};
	struct SceneComponent
	{
		SceneComponent(SceneType scene) : scene(scene) {}
		SceneType scene;
	};

	class Scene
	{
	public:
		Scene(SceneType scene);
		Scene(const Scene& other) = delete;
		Scene& operator=(const Scene& other) = delete;
		virtual ~Scene();

		virtual void SetUpScene() = 0;
		[[nodiscard]] entity CreateEntity() const noexcept;
		SceneType GetSceneType() const noexcept;
	protected:
		SceneType m_sceneType;
		static EntityManager& s_entityManager;
	};
}
