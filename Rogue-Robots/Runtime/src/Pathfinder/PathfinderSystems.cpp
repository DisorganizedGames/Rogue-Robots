#include "PathfinderSystems.h"
#include "Pathfinder.h"

using namespace DOG;

void PathfinderWalkAgentsSystem::OnLateUpdate(DOG::entity e, PathfinderWalkComponent& pathfinder, AgentMovementComponent& movement,
	DOG::RigidbodyComponent& rb, DOG::TransformComponent& trans)
{
	std::vector<Vector3> path = Pathfinder::Get().Checkpoints(trans.GetPosition(), pathfinder.goal);

#ifdef _DEBUG
	Vector3 s = trans.GetPosition();
	std::cout << EntityManager::Get().GetComponent<AgentIdComponent>(e).id << " -> (" << s.x << "," << s.y << "," << s.z << ")";
	for (Vector3 p : path)
	{
		std::cout << " -> (" << p.x << "," << p.y << "," << p.z << ") ";
	}
	std::cout << std::endl;
#endif

	trans.worldMatrix = Matrix::CreateLookAt(trans.GetPosition(), pathfinder.goal, Vector3::Up).Invert();
	
	// TODO: transfer actual movement responsibility to Pathfinder
	constexpr f32 SKID_FACTOR = 0.1f;
	movement.forward.x += rb.linearVelocity.x * SKID_FACTOR;
	movement.forward.y = 0.0f;
	movement.forward.z += rb.linearVelocity.z * SKID_FACTOR;
	movement.forward.Normalize();
	movement.forward *= movement.currentSpeed;
	rb.linearVelocity.x = movement.forward.x;
	rb.linearVelocity.z = movement.forward.z;

	EntityManager::Get().RemoveComponent<PathfinderWalkComponent>(e);
}
