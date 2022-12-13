#include "PathfinderComponents.h"
#include "Game/PCG/PcgLevelLoader.h"

using namespace DOG;
using Vector3 = DirectX::SimpleMath::Vector3;


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

bool NavSceneComponent::HasNavMesh(size_t x, size_t y, size_t z)
{
	if (y < map.size())
		if (z < map[y].size())
			if (x < map[y][z].size())
				return map[y][z][x] != NULL_ENTITY;

	return false;
}

bool NavSceneComponent::HasNavMesh(int x, int y, int z)
{
	if (x < 0 || y < 0 || z < 0)
		return false;

	return HasNavMesh(static_cast<size_t>(x), static_cast<size_t>(y), static_cast<size_t>(z));
}

DOG::entity NavSceneComponent::At(size_t x, size_t y, size_t z)
{
	if (y < map.size())
		if (z < map[y].size())
			if (x < map[y][z].size())
				return map[y][z][x];

	return NULL_ENTITY;
}

DOG::entity NavSceneComponent::At(int x, int y, int z)
{
	if (x < 0 || y < 0 || z < 0)
		return NULL_ENTITY;

	return At(static_cast<size_t>(x), static_cast<size_t>(y), static_cast<size_t>(z));
}

DOG::entity NavSceneComponent::At(Vector3 pos)
{
	int x = static_cast<int>(pos.x / pcgBlock::DIMENSION);
	int y = static_cast<int>(pos.y / pcgBlock::DIMENSION);
	int z = static_cast<int>(pos.z / pcgBlock::DIMENSION);
	NavMeshID id = At(x, y, z);
	return id;
}

bool NavMeshComponent::Connected(NavMeshID mesh1, NavMeshID mesh2)
{
	EntityManager& em = EntityManager::Get();

	for (PortalID portal : portals)
	{
		if (em.GetComponent<PortalComponent>(portal).Connects(mesh1, mesh2))
			return true;
	}
	return false;
}

float NavMeshComponent::CostWalk(const Vector3 enter, const Vector3 exit)
{
	return (exit - enter).Length();
}


PortalComponent::PortalComponent(NavMeshID mesh1, NavMeshID mesh2) : navMesh1(mesh1), navMesh2(mesh2)
{
	EntityManager& em = EntityManager::Get();

	BoundingBoxComponent& bb1 = em.GetComponent<BoundingBoxComponent>(mesh1);
	BoundingBoxComponent& bb2 = em.GetComponent<BoundingBoxComponent>(mesh2);

	// find the point on the border between bb1 and bb2
	portal = (bb2.Center() + bb1.Center()) / 2;
}

PortalComponent::PortalComponent(NavMeshID mesh, Vector3 pos) : navMesh1(mesh), navMesh2(mesh), portal(pos)
{
}


bool PortalComponent::Connects(NavMeshID mesh1, NavMeshID mesh2)
{
	return (mesh1 == navMesh1 && mesh2 == navMesh2) || (mesh1 == navMesh2 && mesh2 == navMesh1);
}

