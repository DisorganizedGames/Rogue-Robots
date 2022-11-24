#include "PathfinderSystems.h"
#include "Pathfinder.h"

using namespace DOG;

void PathfinderWalkAgentsSystem::OnLateUpdate(DOG::entity e, PathfinderWalkComponent& pathfinder, 
	AgentMovementComponent& movement, DOG::RigidbodyComponent& rb, DOG::TransformComponent& trans)
{
	constexpr Vector3 COLOR = Vector3(0.1f, 2.f, 0.1f);

	EntityManager& em = EntityManager::Get();

	std::vector<Vector3> path = Pathfinder::Get().Checkpoints(trans.GetPosition(), pathfinder.goal);

#ifdef _DEBUG
	Vector3 start = trans.GetPosition();
	for (Vector3 end : path)
	{
		entity id = em.CreateEntity();
		auto& laser = em.AddComponent<LaserBeamVFXComponent>(id);
		laser.startPos = start;
		laser.endPos = end;
		laser.color = COLOR;
		start = end;
		em.AddComponent<VisualizePathComponent>(id);
	}
#endif

	trans.worldMatrix = Matrix::CreateLookAt(trans.GetPosition(), path[0], Vector3::Up).Invert();
	//movement.forward = path[0] - trans.GetPosition();
	//movement.forward.Normalize();
	//constexpr f32 SKID_FACTOR = 0.1f;
	//movement.forward.x += rb.linearVelocity.x * SKID_FACTOR;
	//movement.forward.y = 0.0f;
	//movement.forward.z += rb.linearVelocity.z * SKID_FACTOR;
	//movement.forward.Normalize();
	//movement.forward *= movement.currentSpeed;
	//rb.linearVelocity.x = movement.forward.x;
	//rb.linearVelocity.z = movement.forward.z;

	em.RemoveComponent<PathfinderWalkComponent>(e);
}

void VisualizePathCleanUpSystem::OnLateUpdate(DOG::entity e, LaserBeamVFXComponent&, VisualizePathComponent&)
{
	EntityManager::Get().AddComponent<DeferredDeletionComponent>(e);
}

