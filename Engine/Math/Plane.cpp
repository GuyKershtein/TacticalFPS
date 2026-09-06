#include "Plane.h"

namespace Engine {

Plane Plane::FromPoints(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    Plane plane;
    plane.normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
    plane.distance = glm::dot(plane.normal, p0);
    return plane;
}

float Plane::SignedDistance(const glm::vec3& point) const {
    return glm::dot(normal, point) - distance;
}

} // namespace Engine
