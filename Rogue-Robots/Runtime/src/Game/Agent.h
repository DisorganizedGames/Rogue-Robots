#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"


class Agent
{
public:
	Agent();
	~Agent();

private:
	DOG::EntityManager& m_entityManager;
	DOG::entity m_agentEntity;
};
