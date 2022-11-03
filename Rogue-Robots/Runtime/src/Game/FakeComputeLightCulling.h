#pragma once
#include <DOGEngine.h>





class FakeCompute
{
public:

	struct Sphere
	{
		DirectX::SimpleMath::Vector3 center;
		float radius;
		bool culled = false;
	};

	struct Data
	{
		DirectX::SimpleMath::Matrix proj;
		DirectX::SimpleMath::Matrix view;


		std::vector<Sphere> spheres;
	};

	static void Dispatch(int groupCountX, int groupCountY, int groupCountZ);

	inline static int s_groupSizeX = 4;
	inline static int s_groupSizeY = 4;
	inline static int s_groupSizeZ = 1;
	inline static Data s_data;





private:
	static void MainCS(int tidX, int tidY, int tidZ, int gidX, int gidY, int gidZ);
};