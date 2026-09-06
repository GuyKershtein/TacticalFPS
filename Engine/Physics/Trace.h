#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Engine {

// The result of sweeping a point from A to B against level geometry.
struct TraceResult {
    bool hit = false;
    float fraction = 1.0f;             // 0..1 along the requested move; 1.0 = reached the end with nothing in the way
    glm::vec3 endPosition{0.0f};
    glm::vec3 planeNormal{0.0f, 1.0f, 0.0f};
    std::string material;              // the hit face's material name; empty if there was no hit
};

} // namespace Engine
