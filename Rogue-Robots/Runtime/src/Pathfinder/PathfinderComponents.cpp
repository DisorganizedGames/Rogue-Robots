#include "PathfinderComponents.h"

using namespace DOG;


void NavSceneComponent::AddIdAt(size_t x, size_t y, size_t z, entity e)
{
	// expand map if necessary
	while (!(y < map.size()))
		map.emplace_back(std::vector<std::vector<entity>>());
	while (!(z < map[y].size()))
		map[y].emplace_back(std::vector<entity>());
	while (!(x < map[y][z].size()))
		map[y][z].emplace_back(NULL_ENTITY);

	// save block entity id
	map[y][z][x] = e;
}

bool NavSceneComponent::HasNavMesh(int x, int y, int z)
{
	if (x < 0 || y < 0 || z < 0)
		return false;

	size_t ux = static_cast<size_t>(x);
	size_t uy = static_cast<size_t>(y);
	size_t uz = static_cast<size_t>(z);

	if (uy < map.size())
		if (uz < map[uy].size())
			if (ux < map[uy][uz].size())
				return map[uy][uz][ux] != NULL_ENTITY;

	return false;
}

DOG::entity NavSceneComponent::At(size_t x, size_t y, size_t z)
{
	return map[y][z][x];
}

DOG::entity NavSceneComponent::At(int x, int y, int z)
{
#ifdef _DEBUG
	assert(HasNavMesh(x, y, z));
#endif
	return At(static_cast<size_t>(x), static_cast<size_t>(y), static_cast<size_t>(z));
}

bool NavMeshComponent::Connected(NavMeshID mesh1, NavMeshID mesh2)
{
	EntityManager& em = EntityManager::Get();

	for (PortalID portal : portals)
	{
		if (em.GetComponent<PortalComponent>(portal).Connects(mesh1, mesh2))
		{
			//std::cout << "true" << std::endl;
			return true;
		}
	}
	//std::cout << "false" << std::endl;
	return false;
}

PortalComponent::PortalComponent(NavMeshID mesh1, NavMeshID mesh2) : navMesh1(mesh1), navMesh2(mesh2)
{
	EntityManager& em = EntityManager::Get();

	BoundingBoxComponent& bb1 = em.GetComponent<BoundingBoxComponent>(mesh1);
	BoundingBoxComponent& bb2 = em.GetComponent<BoundingBoxComponent>(mesh2);

	// find the point on the border between bb1 and bb2
	portal = (bb2.Center() + bb1.Center()) / 2;
}


bool PortalComponent::Connects(NavMeshID mesh1, NavMeshID mesh2)
{
	return (mesh1 == navMesh1 && mesh2 == navMesh2) || (mesh1 == navMesh2 && mesh2 == navMesh1);
}

