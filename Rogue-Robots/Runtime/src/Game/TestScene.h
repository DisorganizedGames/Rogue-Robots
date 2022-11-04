#pragma once
#include <DOGEngine.h>
#include "Scene.h"

class TestScene : public Scene
{
public:
	TestScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;
};