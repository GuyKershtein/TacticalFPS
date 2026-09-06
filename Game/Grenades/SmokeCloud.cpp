#include "SmokeCloud.h"

#include <algorithm>

namespace Game {

bool SmokeCloud::BlocksSightline(const glm::vec3& from, const glm::vec3& to) const {
    const glm::vec3 segment = to - from;
    const float segmentLengthSq = glm::dot(segment, segment);
    if (segmentLengthSq < 0.0001f) {
        return glm::length(from - position) <= radius;
    }

    const float t = std::clamp(glm::dot(position - from, segment) / segmentLengthSq, 0.0f, 1.0f);
    const glm::vec3 closestPoint = from + segment * t;
    return glm::length(closestPoint - position) <= radius;
}

} // namespace Game
