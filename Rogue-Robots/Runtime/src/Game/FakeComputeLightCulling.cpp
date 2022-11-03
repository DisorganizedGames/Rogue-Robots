#include "FakeComputeLightCulling.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

void FakeCompute::Dispatch(int groupCountX, int groupCountY, int groupCountZ)
{
	for (int groupX = 0; groupX < groupCountX; groupX++)
	{
		for (int groupY = 0; groupY < groupCountY; groupY++)
		{
			for (int groupZ = 0; groupZ < groupCountZ; groupZ++)
			{
				for (int threadX = 0; threadX < s_groupSizeX; threadX++)
				{
					for (int threadY = 0; threadY < s_groupSizeY; threadY++)
					{
						for (int threadZ = 0; threadZ < s_groupSizeZ; threadZ++)
						{
							MainCS(threadX, threadY, threadZ, groupX, groupY, groupZ);
						}
					}
				}
			}
		}
	}
}

void FakeCompute::MainCS(int tidX, int tidY, int tidZ, int gidX, int gidY, int gidZ)
{
	Vector2i tile{ gidX, gidY };
	std::cout << "tidX: " << tidX << ", tidY: " << tidY << ", tidZ: " << tidZ << ", gidX: " << gidX << ", gidY: " << gidY << ", gidZ: " << gidZ << std::endl;




}