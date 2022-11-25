#include "PathfinderSystems.h"
#include "Pathfinder.h"

using namespace DOG;

void PathfinderWalkSystem::OnUpdate(PathfinderWalkComponent& pfc, DOG::TransformComponent& trans)
{
	constexpr Vector3 LASER_COLOR = Vector3(0.1f, 2.f, 0.1f);

	EntityManager& em = EntityManager::Get();

	Pathfinder::Get().Checkpoints(trans.GetPosition(), pfc.goal, pfc);

#ifdef _DEBUG
	Vector3 start = trans.GetPosition();
	for (Vector3 end : pfc.path)
	{
		entity id = em.CreateEntity();
		auto& laser = em.AddComponent<LaserBeamVFXComponent>(id);
		laser.startPos = start;
		laser.endPos = end;
		laser.color = LASER_COLOR;
		start = end;
		em.AddComponent<VisualizePathComponent>(id);
	}
#endif
}

void VisualizePathCleanUpSystem::OnUpdate(DOG::entity e, LaserBeamVFXComponent&, VisualizePathComponent&)
{
	//EntityManager::Get().DestroyEntity(e);
	EntityManager::Get().AddComponent<DeferredDeletionComponent>(e);
}

