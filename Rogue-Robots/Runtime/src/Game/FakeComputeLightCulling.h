#pragma once
#include <DOGEngine.h>




class LightScene;
class FakeCompute
{
public:
	struct Sphere
	{
		DOG::entity e;
		DirectX::SimpleMath::Vector4 center;
		float radius;
		bool culled = false;
	};

	struct Data
	{
		DirectX::SimpleMath::Matrix proj;
		DirectX::SimpleMath::Matrix view;
		DirectX::SimpleMath::Vector2 res;

		std::vector<Sphere> spheres;
	};

	void Dispatch(int groupCountX, int groupCountY, int groupCountZ);

	int m_groupSizeX = 4;
	int m_groupSizeY = 4;
	int m_groupSizeZ = 1;
	Data m_data;

	LightScene* m_lightScene = nullptr;



private:
	void MainCS(int tidX, int tidY, int tidZ, int gidX, int gidY, int gidZ);
};