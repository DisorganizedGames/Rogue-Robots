#pragma once
#include <DOGEngine.h>
#include "../Scene.h"

class ParticleScene : public Scene
{
public:
	ParticleScene();
	~ParticleScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

private:
	DOG::entity m_particleSystem = DOG::NULL_ENTITY;

private:
	void ParticleSystemMenu(bool& open);
	
};