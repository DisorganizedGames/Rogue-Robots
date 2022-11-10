#include "PcgLevelLoader.h"
#include <DOGEngine.h>
using namespace DOG;
using namespace DirectX::SimpleMath;

std::vector<DOG::entity> LoadLevel(std::string file)
{
	auto& em = EntityManager::Get();

	float blockDim = 4.6f;

	std::string line;

	std::ifstream inputFile(file);

	AssetManager& aManager = AssetManager::Get();

	std::vector<entity> levelBlocks;

	unsigned x = 0;
	unsigned y = 0;
	unsigned z = 0;
	float piDiv2 = DirectX::XM_PIDIV2;
	if (inputFile.is_open())
	{
		//Read the room data
		do
		{
			std::getline(inputFile, line);
			//Do stuff with room data here.
		} while (line != "");
		//Read the level
		while (std::getline(inputFile, line))
		{
			if (line[0] != '-')
			{
				while (line.find(' ') != std::string::npos)
				{
					size_t delimPos = line.find(' ');
					std::string block = line.substr(0, delimPos);
					line.erase(0, delimPos + 1);
					if (block != "Empty" && block != "Void")
					{
						size_t firstUnderscore = block.find('_');
						size_t secondUnderscore = block.find('_', firstUnderscore + 1);
						std::string blockName = block.substr(0, firstUnderscore);
						int blockRot = std::stoi(block.substr(firstUnderscore + 2, secondUnderscore - firstUnderscore - 2));
						Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);

						entity blockEntity = levelBlocks.emplace_back(em.CreateEntity());
						em.AddComponent<ModelComponent>(blockEntity, aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + ".gltf"));
						em.AddComponent<TransformComponent>(blockEntity,
							Vector3(x * blockDim, y * blockDim, z * blockDim),
							Vector3(0.0f, -blockRot * piDiv2, 0.0f),
							scale);

						em.AddComponent<ModularBlockComponent>(blockEntity);
						em.AddComponent<MeshColliderComponent>(blockEntity,
							blockEntity,
							aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + "_Col.gltf", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::CPUMemory | DOG::AssetLoadFlag::GPUMemory))),
							scale,
							false);		// Set this to true if you want to see colliders only in wireframe
						em.AddComponent<ShadowReceiverComponent>(blockEntity);
					}
					++z;
				}
				z = 0;
				++y;
			}
			else
			{
				z = 0;
				y = 0;
				++x;
			}
		}
	}
	return levelBlocks;
}
