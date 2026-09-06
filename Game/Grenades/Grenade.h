#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "GrenadeTypes.h"
#include "../../Engine/World/Brush.h"

namespace Game {

// Physics shared by every grenade type: an arc trajectory that bounces off
// world geometry and loses energy each bounce, ticking down a fuse. What
// happens ON detonation differs per kind (damage vs. smoke vs. blind vs.
// fake gunfire) — that's the caller's job (see
// GameApplication::DetonateGrenade), since Grenade itself doesn't know
// about Damageable/AudioSystem/ParticleSystem/bots.
class Grenade {
public:
    void Init(const glm::vec3& position, const glm::vec3& velocity, GrenadeKind kind);

    // Returns true exactly on the frame the fuse expires.
    bool Update(float deltaTime, const std::vector<Engine::Brush>& worldBrushes);

    GrenadeKind GetKind() const { return m_kind; }
    const glm::vec3& GetPosition() const { return m_position; }
    const GrenadeData& GetData() const { return m_data; }

private:
    glm::vec3 m_position{0.0f};
    glm::vec3 m_velocity{0.0f};
    float m_fuseRemaining = 0.0f;
    GrenadeKind m_kind = GrenadeKind::Fragmentation;
    GrenadeData m_data;

    static constexpr float kGravity = 800.0f;
    static constexpr float kRestitution = 0.45f; // fraction of into-surface speed kept (reflected) after a bounce
};

} // namespace Game
