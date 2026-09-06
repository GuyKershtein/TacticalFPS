#include "CollisionWorld.h"

namespace Engine {

void CollisionWorld::Build(const std::vector<Brush>& worldBrushes) {
    m_sourceBrushes = worldBrushes;

    m_standingExpanded.clear();
    m_crouchingExpanded.clear();
    m_standingExpanded.reserve(worldBrushes.size());
    m_crouchingExpanded.reserve(worldBrushes.size());

    const CollisionHull standing = CollisionHull::Standing();
    const CollisionHull crouching = CollisionHull::Crouching();

    for (const Brush& brush : m_sourceBrushes) {
        m_standingExpanded.push_back(brush.ExpandForHull(standing.halfExtents));
        m_crouchingExpanded.push_back(brush.ExpandForHull(crouching.halfExtents));
    }
}

} // namespace Engine
