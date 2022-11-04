#pragma once
#include <DOGEngine.h>
#include "Scene.h"

class MainScene : public Scene
{
public:
	MainScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;
};