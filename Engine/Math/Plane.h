#pragma once

#include <glm/glm.hpp>

namespace Engine {

// A plane in point-normal form: points p with dot(normal, p) == distance lie
// on the plane. SignedDistance(p) > 0 means p is in front of (outside) the
// plane; <= 0 means behind (inside, for a brush's solid volume).
struct Plane {
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;

    // Builds a plane from three points on it. Winding matters: normal =
    // normalize(cross(p1 - p0, p2 - p0)), so point order determines which
    // side is "outside".
    static Plane FromPoints(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2);

    float SignedDistance(const glm::vec3& point) const;
};

} // namespace Engine
