#include "FakeComputeLightCulling.h"

#include "LightScene.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

void FakeCompute::Dispatch(int groupCountX, int groupCountY, int groupCountZ)
{
	m_data.localLightBuffers.clear();
	m_data.localLightBuffers.resize(groupCountX * groupCountY * groupCountZ);
	for (int groupX = 0; groupX < groupCountX; groupX++)
	{
		for (int groupY = 0; groupY < groupCountY; groupY++)
		{
			for (int groupZ = 0; groupZ < groupCountZ; groupZ++)
			{
				for (int threadX = 0; threadX < m_groupSizeX; threadX++)
				{
					for (int threadY = 0; threadY < m_groupSizeY; threadY++)
					{
						for (int threadZ = 0; threadZ < m_groupSizeZ; threadZ++)
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
	auto frustumPlanes = m_lightScene->ExtractPlanes(m_data.proj, m_data.view, (int)m_data.res.x, (int)m_data.res.y, m_groupSizeX, tile);
	if (tidX == 0 && tidY == 0)
	{
		if (frustumPlanes.size() == 6)
		{
			m_lightScene->AddFrustum(frustumPlanes[0], frustumPlanes[1], frustumPlanes[2], frustumPlanes[3], frustumPlanes[4], frustumPlanes[5]);
		}
	}


	int tid = tidX + m_groupSizeX * tidY + m_groupSizeX * m_groupSizeZ * tidZ;
	if (tid >= m_data.spheres.size()) return;

	int gid = gidX + m_data.groupCountX * gidY + m_data.groupCountX * m_data.groupCountY * gidZ;
	int threadCount = m_groupSizeX * m_groupSizeY * m_groupSizeZ;
	for (int i = tid; i < m_data.spheres.size(); i += threadCount)
	{
		auto& sphere = m_data.spheres[i];
		bool culled = false;
		for (int j = 0; j < 6; j++)
		{
			float d = sphere.center.Dot(frustumPlanes[j]);
			culled |= d < -sphere.radius;
		}
		if (!culled)
		{
			m_data.localLightBuffers[gid].push_back(i);
		}
	}
}
