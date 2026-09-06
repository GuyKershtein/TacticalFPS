#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Damageable.h"
#include "../../Engine/World/Brush.h"

namespace Game {

// Anything a hitscan ray can hit besides world geometry: a sphere collider
// (this milestone's stand-in for real hitboxes — see TargetDummy) tagged
// with which zone it represents and where to deliver damage.
struct HitscanTarget {
    glm::vec3 center;
    float radius;
    HitZone zone;
    Damageable* damageable;
};

struct HitscanResult {
    bool hit = false;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    Damageable* hitDamageable = nullptr;
    HitZone hitZone = HitZone::Body;
    float distance = 0.0f;
    std::string material; // the world surface material hit; empty if a Damageable was hit instead
};

// Traces one ray against world geometry and a set of target spheres,
// returning whichever is closer. World geometry always blocks the shot —
// bullets don't pass through walls to reach a target behind them; targets
// are resolved by simple ray-sphere intersection since there's no skeletal
// hitbox system yet.
HitscanResult ResolveHitscan(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
    const std::vector<Engine::Brush>& worldBrushes, const std::vector<HitscanTarget>& targets);

} // namespace Game
