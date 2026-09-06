#pragma once

#include <vector>
#include "Brush.h"
#include "CollisionHull.h"

namespace Engine {

// Holds a level's collision brushes and the precomputed hull-expanded
// versions of them. This milestone only builds that data; the actual
// sweep/clip movement queries (TraceBox, ClipVelocity) belong to the player
// controller in Milestone 2, once there's a real controller to exercise and
// test them against.
class CollisionWorld {
public:
    void Build(const std::vector<Brush>& worldBrushes);

    const std::vector<Brush>& GetRawBrushes() const { return m_sourceBrushes; }
    const std::vector<Brush>& GetStandingHullBrushes() const { return m_standingExpanded; }
    const std::vector<Brush>& GetCrouchingHullBrushes() const { return m_crouchingExpanded; }

private:
    std::vector<Brush> m_sourceBrushes;
    std::vector<Brush> m_standingExpanded;
    std::vector<Brush> m_crouchingExpanded;
};

} // namespace Engine
