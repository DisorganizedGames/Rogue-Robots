#pragma once
#include <DOGEngine.h>
#include "../Scene.h"

class ParticleScene : public Scene
{
public:
	ParticleScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;
};