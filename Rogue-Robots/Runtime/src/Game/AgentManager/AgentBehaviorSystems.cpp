#include "AgentBehaviorSystems.h"

using namespace DOG;

//void AgentBehaviorSystem::Push(entity e, AgentBehavior b, AgentBehaviorComponent& comp, bool netSync)
//{
//	if ((comp.current + 1) < comp.stack.max_size())
//		comp.stack[++comp.current] = b;
//	if (netSync)
//	{
//		NetworkAgentBehaviorSync& sync = EntityManager::Get().AddComponent<NetworkAgentBehaviorSync>(e);
//		sync.agentID = ;
//	}
//}