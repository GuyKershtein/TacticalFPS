#include "NavGraph.h"
#include "../Physics/CollisionQuery.h"

#include <queue>
#include <algorithm>
#include <limits>

namespace Engine {

void NavGraph::Build(const std::vector<glm::vec3>& waypoints, const std::vector<Brush>& worldBrushes) {
    m_waypoints = waypoints;
    m_edges.assign(m_waypoints.size(), {});

    for (size_t i = 0; i < m_waypoints.size(); ++i) {
        for (size_t j = i + 1; j < m_waypoints.size(); ++j) {
            const glm::vec3 delta = m_waypoints[j] - m_waypoints[i];
            const float distance = glm::length(delta);
            if (distance > kMaxLinkDistance) continue;

            const glm::vec3 raisedA = m_waypoints[i] + glm::vec3(0.0f, kEyeHeightOffset, 0.0f);
            const glm::vec3 raisedB = m_waypoints[j] + glm::vec3(0.0f, kEyeHeightOffset, 0.0f);
            if (TraceMove(raisedA, raisedB, worldBrushes).hit) continue; // blocked by a wall

            m_edges[i].push_back({static_cast<int>(j), distance});
            m_edges[j].push_back({static_cast<int>(i), distance});
        }
    }
}

int NavGraph::FindNearestWaypoint(const glm::vec3& position) const {
    int nearest = -1;
    float nearestDistSq = std::numeric_limits<float>::max();
    for (size_t i = 0; i < m_waypoints.size(); ++i) {
        const glm::vec3 diff = m_waypoints[i] - position;
        const float distSq = glm::dot(diff, diff);
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearest = static_cast<int>(i);
        }
    }
    return nearest;
}

std::vector<int> NavGraph::FindPath(const glm::vec3& fromPosition, const glm::vec3& toPosition) const {
    if (m_waypoints.empty()) return {};

    const int start = FindNearestWaypoint(fromPosition);
    const int goal = FindNearestWaypoint(toPosition);
    if (start < 0 || goal < 0) return {};
    if (start == goal) return {start};

    // Standard A* with a straight-line-distance heuristic.
    struct OpenEntry {
        int index;
        float priority;
        bool operator>(const OpenEntry& other) const { return priority > other.priority; }
    };

    std::vector<float> gScore(m_waypoints.size(), std::numeric_limits<float>::max());
    std::vector<int> cameFrom(m_waypoints.size(), -1);
    std::vector<bool> visited(m_waypoints.size(), false);
    std::priority_queue<OpenEntry, std::vector<OpenEntry>, std::greater<OpenEntry>> open;

    const auto heuristic = [&](int index) { return glm::length(m_waypoints[index] - m_waypoints[goal]); };

    gScore[start] = 0.0f;
    open.push({start, heuristic(start)});

    while (!open.empty()) {
        const int current = open.top().index;
        open.pop();
        if (visited[current]) continue;
        visited[current] = true;

        if (current == goal) break;

        for (const Edge& edge : m_edges[current]) {
            const float tentativeG = gScore[current] + edge.cost;
            if (tentativeG < gScore[edge.targetIndex]) {
                gScore[edge.targetIndex] = tentativeG;
                cameFrom[edge.targetIndex] = current;
                open.push({edge.targetIndex, tentativeG + heuristic(edge.targetIndex)});
            }
        }
    }

    if (!visited[goal]) return {}; // no path

    std::vector<int> path;
    for (int at = goal; at != -1; at = cameFrom[at]) {
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::pair<int, int>> NavGraph::GetDebugEdges() const {
    std::vector<std::pair<int, int>> edges;
    for (size_t i = 0; i < m_edges.size(); ++i) {
        for (const Edge& edge : m_edges[i]) {
            // Build() always adds both directions of a link at once, so
            // only keeping the (i < target) direction reports each
            // undirected edge exactly once.
            if (edge.targetIndex > static_cast<int>(i)) {
                edges.emplace_back(static_cast<int>(i), edge.targetIndex);
            }
        }
    }
    return edges;
}

} // namespace Engine
