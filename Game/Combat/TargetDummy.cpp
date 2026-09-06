#include "TargetDummy.h"

#include <cstdio>

namespace Game {

void TargetDummy::OnSpawn() {
    m_origin = GetVector("origin");
    m_maxHealth = GetFloat("health", 100.0f);
    m_health = m_maxHealth;
}

void TargetDummy::Update(float deltaTime) {
    if (m_health > 0.0f) return;

    m_respawnTimer -= deltaTime;
    if (m_respawnTimer <= 0.0f) {
        m_health = m_maxHealth;
        std::printf("[TargetDummy] Respawned\n");
        std::fflush(stdout);
    }
}

void TargetDummy::TakeDamage(float amount, HitZone zone) {
    if (!IsAlive()) return;

    m_health -= amount;
    const char* zoneName = zone == HitZone::Head ? "HEAD" : zone == HitZone::Legs ? "LEGS" : "BODY";

    if (m_health <= 0.0f) {
        m_health = 0.0f;
        m_respawnTimer = kRespawnDelaySeconds;
        std::printf("[TargetDummy] Killed with a %s hit\n", zoneName);
    } else {
        std::printf("[TargetDummy] Took %.1f damage to the %s (%.0f/%.0f HP)\n", amount, zoneName, m_health, m_maxHealth);
    }
    std::fflush(stdout);
}

void TargetDummy::CollectHitscanTargets(std::vector<HitscanTarget>& outTargets) {
    if (!IsAlive()) return;

    // Feet at m_origin (matching info_player_start's own convention);
    // legs/body/head stacked upward from there.
    outTargets.push_back({m_origin + glm::vec3(0.0f, kLegsRadius, 0.0f), kLegsRadius, HitZone::Legs, this});
    outTargets.push_back({m_origin + glm::vec3(0.0f, kLegsRadius * 2.0f + kBodyRadius, 0.0f), kBodyRadius, HitZone::Body, this});
    outTargets.push_back({m_origin + glm::vec3(0.0f, kLegsRadius * 2.0f + kBodyRadius * 2.0f + kHeadRadius, 0.0f), kHeadRadius, HitZone::Head, this});
}

REGISTER_ENTITY_CLASS("target_dummy", TargetDummy);

} // namespace Game
