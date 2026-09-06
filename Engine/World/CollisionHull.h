#pragma once

#include <glm/glm.hpp>

namespace Engine {

// Named half-extent presets for GoldSrc-style hull-expansion collision:
// rather than testing the player's actual box against level brushes, the
// brushes are expanded outward by the hull's half-extents once (see
// Brush::ExpandForHull), and movement then only needs a point-vs-expanded-
// brush test. Components are (halfWidthX, halfHeightY, halfWidthZ) to match
// this engine's Y-up convention.
struct CollisionHull {
    glm::vec3 halfExtents;

    static CollisionHull Standing() { return {glm::vec3(16.0f, 36.0f, 16.0f)}; }  // ~32x32 footprint, 72 tall
    static CollisionHull Crouching() { return {glm::vec3(16.0f, 18.0f, 16.0f)}; } // ~32x32 footprint, 36 tall
};

} // namespace Engine
