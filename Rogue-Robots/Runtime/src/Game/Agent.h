#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

extern DOG::entity g_newestAgent;

class Agent
{
public:
	Agent();
	~Agent();
	DOG::entity MakeAgent(DOG::entity e) noexcept;
private:
	bool m_useNetworking = true;
	DOG::EntityManager& m_entityManager;
};
