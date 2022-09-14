#pragma once
#include <DOGEngine.h>
#include <vector>


class Pathfinder : public DOG::Layer
{
using Vector3 = DirectX::SimpleMath::Vector3;
public:
	Pathfinder() noexcept;
	virtual ~Pathfinder() override final = default;
	virtual void OnAttach() override final;
	virtual void OnDetach() override final;
	virtual void OnUpdate() override final;
	virtual void OnRender() override final;
	virtual void OnEvent(DOG::IEvent& event) override final;

	// newPos Walk(currentPos, goal, speed)
	// newPos Fly(currentPos, goal, speed)

private:
	struct TransitPlane
	{
		Vector3	lowCorner;
		Vector3	hiCorner;
		size_t  iMesh1;
		size_t  iMesh2;
		TransitPlane(Vector3 low, Vector3 hi, size_t one, size_t two) : lowCorner(low), hiCorner(hi), iMesh1(one), iMesh2(two) {}
		TransitPlane(Vector3 pos, size_t meshIdx) : lowCorner(pos), hiCorner(pos), iMesh1(meshIdx), iMesh2(meshIdx) {}

		// Methods
		Vector3 Midpoint();
	};
	struct NavMesh
	{
		// Extents
		Vector3 lowCorner;
		Vector3 hiCorner;
		// Exit zones
		std::vector<size_t> iTransits;

		// Methods
		NavMesh(Vector3 low, Vector3 hi);
		void AddTransitPlane(size_t iTransit);
		bool Contains(const Vector3 pos);
		float CostWalk(const Vector3 enter, const Vector3 exit);
		float CostFly(const Vector3 enter, const Vector3 exit);
	};

	// The graph representation of the map
	std::vector<NavMesh> m_navMeshes;
	std::vector<TransitPlane> m_transits;

	// Methods
	size_t FindNavMeshContaining(const Vector3 pos);
	std::vector<TransitPlane*> Astar(const Vector3 start, const Vector3 goal, float (*h)(Vector3, Vector3));
	float heuristicStraightLine(Vector3 start, Vector3 goal);
};