#pragma once

#include <glm/glm.hpp>

namespace Game {

// A lingering vision-blocking volume left behind by a detonated smoke
// grenade. Deliberately a simple sphere rather than real volumetric fog —
// bot line-of-sight checks treat "sightline passes within this radius of
// the center" as blocked, a cheap approximation appropriate to this
// project's fidelity (see BotPerception::CanSee).
struct SmokeCloud {
    glm::vec3 position{0.0f};
    float radius = 220.0f;
    float remainingSeconds = 0.0f;

    bool BlocksSightline(const glm::vec3& from, const glm::vec3& to) const;
};

} // namespace Game
