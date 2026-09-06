#include "BotPerception.h"
#include "../../Engine/Physics/CollisionQuery.h"

#include <cmath>

namespace Game {
namespace BotPerception {

bool CanSee(const glm::vec3& eyePosition, const glm::vec3& forward, const glm::vec3& targetPosition,
    const BotDifficulty& difficulty, const std::vector<Engine::Brush>& worldBrushes,
    const std::vector<SmokeCloud>& smokeClouds) {
    const glm::vec3 toTarget = targetPosition - eyePosition;
    const float distance = glm::length(toTarget);
    if (distance > difficulty.visionRangeUnits || distance < 0.0001f) return false;

    const glm::vec3 toTargetDir = toTarget / distance;
    const float cosAngle = glm::dot(glm::normalize(forward), toTargetDir);
    const float cosHalfFov = std::cos(glm::radians(difficulty.visionFovDegrees * 0.5f));
    if (cosAngle < cosHalfFov) return false;

    for (const SmokeCloud& cloud : smokeClouds) {
        if (cloud.BlocksSightline(eyePosition, targetPosition)) return false;
    }

    const Engine::TraceResult trace = Engine::TraceMove(eyePosition, targetPosition, worldBrushes);
    return !trace.hit;
}

const SoundEvent* FindAudibleSound(const glm::vec3& earPosition, const std::vector<SoundEvent>& events, const BotDifficulty& difficulty) {
    const SoundEvent* best = nullptr;
    float bestRatio = 1.0f; // distance/effectiveRadius must be < 1 (audible) and smallest wins

    for (const SoundEvent& event : events) {
        const float effectiveRadius = event.radius * difficulty.hearingRadiusMultiplier;
        if (effectiveRadius <= 0.0f) continue;

        const float distance = glm::length(event.position - earPosition);
        const float ratio = distance / effectiveRadius;
        if (ratio < bestRatio) {
            bestRatio = ratio;
            best = &event;
        }
    }
    return best;
}

} // namespace BotPerception
} // namespace Game
