#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../World/Brush.h"

namespace Engine {

// A waypoint graph for bot pathfinding — the natural fit for small,
// hand-placed tactical maps (a full navmesh would auto-generate walkable
// surface coverage, which needs far more geometry-processing machinery
// than this project has built). Map authors place waypoints; edges between
// them are derived automatically (any two within range with a clear line
// of sight are linked) rather than needing hand-specified connections,
// which would be tedious and easy to get wrong in a text format.
class NavGraph {
public:
    void Build(const std::vector<glm::vec3>& waypoints, const std::vector<Brush>& worldBrushes);

    // A* search over the waypoint graph. Returns the sequence of waypoint
    // indices to walk through (empty if no path exists or the graph is
    // empty). Does not include a synthetic start node — the caller is
    // responsible for walking from its current position to
    // GetWaypointPosition(result[0]) first.
    std::vector<int> FindPath(const glm::vec3& fromPosition, const glm::vec3& toPosition) const;

    int FindNearestWaypoint(const glm::vec3& position) const;
    const glm::vec3& GetWaypointPosition(int index) const { return m_waypoints[index]; }
    size_t GetWaypointCount() const { return m_waypoints.size(); }

    // Debug visualization only (Milestone 9) — every graph edge as a
    // (fromIndex, toIndex) pair, each undirected edge returned once.
    std::vector<std::pair<int, int>> GetDebugEdges() const;

private:
    struct Edge {
        int targetIndex;
        float cost;
    };

    // Two waypoints separated by a wall obviously shouldn't link just for
    // being close together, but a raw line-of-sight test at ground level
    // would also wrongly connect waypoints on opposite sides of a low
    // barrier a bot can't actually step over. Testing at approximate torso
    // height is a simple, adequate proxy without needing full hull-swept
    // clearance checks between every waypoint pair.
    static constexpr float kEyeHeightOffset = 32.0f;
    static constexpr float kMaxLinkDistance = 350.0f;

    std::vector<glm::vec3> m_waypoints;
    std::vector<std::vector<Edge>> m_edges; // m_edges[i] = neighbors of waypoint i
};

} // namespace Engine
