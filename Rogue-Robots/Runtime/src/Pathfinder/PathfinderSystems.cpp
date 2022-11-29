#include "PathfinderSystems.h"
#include "Pathfinder.h"

using namespace DOG;

void PathfinderWalkSystem::OnUpdate(PathfinderWalkComponent& pfc, DOG::TransformComponent& trans)
{
	constexpr Vector3 LASER_COLOR = Vector3(0.1f, 2.f, 0.1f);

	Pathfinder& pf = Pathfinder::Get();

	pf.Checkpoints(trans.GetPosition(), pfc);

	if (pf.DrawPaths())
	{
		EntityManager& em = EntityManager::Get();
		Vector3 start = trans.GetPosition();
		for (Vector3& end : pfc.path)
		{
			entity id = em.CreateEntity();
			auto& laser = em.AddComponent<LaserBeamVFXComponent>(id);
			laser.startPos = start;
			laser.endPos = end;
			laser.color = LASER_COLOR;
			start = end;
			em.AddComponent<VisualizePathComponent>(id);
			em.Collect<ThisPlayer, SceneComponent>().Do(
				[&](ThisPlayer&, SceneComponent& sc)
				{
					em.AddComponent<SceneComponent>(id, sc.scene);
				});
		}
	}
}

void VisualizePathCleanUpSystem::OnUpdate(DOG::entity e, LaserBeamVFXComponent&, VisualizePathComponent&)
{
	EntityManager::Get().DestroyEntity(e);
}

