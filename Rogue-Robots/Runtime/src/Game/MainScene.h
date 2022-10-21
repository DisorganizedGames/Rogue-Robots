#pragma once
#include <DOGEngine.h>

class MainScene : public DOG::Scene
{
public:
	MainScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;
};