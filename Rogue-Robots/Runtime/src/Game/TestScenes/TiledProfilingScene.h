#pragma once
#include <DOGEngine.h>
#include "../Scene.h"

class TiledProfilingScene : public Scene
{
public:
	TiledProfilingScene();
	~TiledProfilingScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;
private:
	void TiledProfilingMenu(bool& open);
};
