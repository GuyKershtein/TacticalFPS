#include "HitscanSystem.h"
#include "../../Engine/Physics/CollisionQuery.h"

#include <cmath>

namespace Game {

namespace {

bool RaySphereIntersect(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& center, float radius, float& outDistance) {
    const glm::vec3 originToCenter = origin - center;
    const float b = glm::dot(originToCenter, direction);
    const float c = glm::dot(originToCenter, originToCenter) - radius * radius;
    const float discriminant = b * b - c;
    if (discriminant < 0.0f) return false;

    const float sqrtDiscriminant = std::sqrt(discriminant);
    const float t0 = -b - sqrtDiscriminant;
    const float t1 = -b + sqrtDiscriminant;
    const float t = t0 >= 0.0f ? t0 : t1;
    if (t < 0.0f) return false;

    outDistance = t;
    return true;
}

} // namespace

HitscanResult ResolveHitscan(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
    const std::vector<Engine::Brush>& worldBrushes, const std::vector<HitscanTarget>& targets) {
    HitscanResult result;

    const glm::vec3 end = origin + direction * maxDistance;
    const Engine::TraceResult worldTrace = Engine::TraceMove(origin, end, worldBrushes);
    const float worldHitDistance = worldTrace.hit ? maxDistance * worldTrace.fraction : maxDistance;

    float closestDistance = worldHitDistance;
    const HitscanTarget* closestTarget = nullptr;

    for (const HitscanTarget& target : targets) {
        if (!target.damageable || !target.damageable->IsAlive()) continue;

        float distance;
        if (RaySphereIntersect(origin, direction, target.center, target.radius, distance) && distance < closestDistance) {
            closestDistance = distance;
            closestTarget = &target;
        }
    }

    if (closestTarget) {
        result.hit = true;
        result.point = origin + direction * closestDistance;
        result.normal = glm::normalize(result.point - closestTarget->center);
        result.hitDamageable = closestTarget->damageable;
        result.hitZone = closestTarget->zone;
        result.distance = closestDistance;
    } else if (worldTrace.hit) {
        result.hit = true;
        result.point = origin + direction * worldHitDistance;
        result.normal = worldTrace.planeNormal;
        result.distance = worldHitDistance;
        result.material = worldTrace.material;
    }

    return result;
}

} // namespace Game
