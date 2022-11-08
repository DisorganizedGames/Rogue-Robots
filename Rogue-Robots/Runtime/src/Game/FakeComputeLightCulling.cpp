#include "FakeComputeLightCulling.h"

#include "LightScene.h"

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

	//int tid = tidX + m_groupSizeX * tidY + m_groupSizeX * m_groupSizeZ * tidZ;

	std::cout << "tidX: " << tidX << ", tidY: " << tidY << ", tidZ: " << tidZ << ", gidX: " << gidX << ", gidY: " << gidY << ", gidZ: " << gidZ << std::endl;

	auto frustumPlanes = m_lightScene->ExtractPlanes(m_data.proj, m_data.view, (int)m_data.res.x, (int)m_data.res.y, m_groupSizeX, tile);
	if (tidX == 0 && tidY == 0)
	{
		if (frustumPlanes.size() == 6)
		{
			std::cout << "AddFrustum: " << std::endl;
			m_lightScene->AddFrustum(frustumPlanes[0], frustumPlanes[1], frustumPlanes[2], frustumPlanes[3], frustumPlanes[4], frustumPlanes[5]);
		}
	}
}



//void FakeCompute::MainCS(int tidX, int tidY, int tidZ, int gidX, int gidY, int gidZ)
//{
//	Vector2i tile{ gidX, gidY };
//
//	int tid = tidX + s_groupSizeX * tidY + s_groupSizeX * s_groupSizeZ * tidZ;
//	if (tid >= s_data.spheres.size()) return;
//
//	//for(int i = 0; i < s_data.spheres.size(); i+= )
//	std::cout << "tidX: " << tidX << ", tidY: " << tidY << ", tidZ: " << tidZ << ", gidX: " << gidX << ", gidY: " << gidY << ", gidZ: " << gidZ << std::endl;
//
//
//	Matrix m = s_data.view * s_data.proj;
//
//	Vector4 leftP = { m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41 };
//	Vector4 rightP = { m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41 };
//	Vector4 botP = { m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42 };
//	Vector4 topP = { m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42 };
//	Vector4 nearP = { m._13, m._23, m._33, m._43 };
//	Vector4 farP = { m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43 };
//
//	leftP = DirectX::XMPlaneNormalize(leftP);
//	rightP = DirectX::XMPlaneNormalize(rightP);
//	botP = DirectX::XMPlaneNormalize(botP);
//	topP = DirectX::XMPlaneNormalize(topP);
//	nearP = DirectX::XMPlaneNormalize(nearP);
//	farP = DirectX::XMPlaneNormalize(farP);
//	if (tid == 0)
//	{
//		// for one thread only
//		for (int i = 0; i < s_data.spheres.size(); i++)
//		{
//			float d;
//			auto& sphere = s_data.spheres[i];
//			d = sphere.center.Dot(leftP);
//			if (d < -sphere.radius)
//			{
//				std::cout << "culled left" << d <<  std::endl;
//				sphere.culled = true;
//			}
//
//			d = sphere.center.Dot(rightP);
//			if (d < -sphere.radius)
//			{
//				std::cout << "culled right" << d << std::endl;
//				sphere.culled = true;
//			}
//
//			d = sphere.center.Dot(nearP);
//			if (d < -sphere.radius)
//			{
//				std::cout << "culled near" << d << std::endl;
//				sphere.culled = true;
//			}
//
//			d = sphere.center.Dot(farP);
//			if (d < -sphere.radius)
//			{
//				std::cout << "culled far" << d << std::endl;
//				sphere.culled = true;
//			}
//
//			d = sphere.center.Dot(topP);
//			if (d < -sphere.radius)
//			{
//				std::cout << "culled top" << d << std::endl;
//				sphere.culled = true;
//			}
//
//			d = sphere.center.Dot(botP);
//			if (d < -sphere.radius)
//			{
//				std::cout << "culled bot" << d << std::endl;
//				sphere.culled = true;
//			}
//		}
//	}
//}
