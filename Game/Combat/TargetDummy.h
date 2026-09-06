#pragma once

#include "../../Engine/World/Entity.h"
#include "Damageable.h"
#include "HitscanSystem.h"
#include <vector>

namespace Game {

// A simple shootable practice target: three stacked sphere colliders
// (head/body/legs) standing in for real hitboxes until there's a skeletal
// model to attach them to. Placed via the map's entity system (classname
// "target_dummy"), proving hit detection end-to-end before Milestone 6
// gives bots the same Damageable interface.
class TargetDummy : public Engine::Entity, public Damageable {
public:
    void OnSpawn() override;
    void Update(float deltaTime);

    void TakeDamage(float amount, HitZone zone) override;
    bool IsAlive() const override { return m_health > 0.0f; }

    // Appends this dummy's current hit spheres (nothing appended if it's
    // dead and waiting to respawn).
    void CollectHitscanTargets(std::vector<HitscanTarget>& outTargets);

    const glm::vec3& GetOrigin() const { return m_origin; }

private:
    glm::vec3 m_origin{0.0f};
    float m_maxHealth = 100.0f;
    float m_health = 100.0f;
    float m_respawnTimer = 0.0f;

    static constexpr float kHeadRadius = 12.0f;
    static constexpr float kBodyRadius = 18.0f;
    static constexpr float kLegsRadius = 16.0f;
    static constexpr float kRespawnDelaySeconds = 3.0f;
};

} // namespace Game
