#include "Grenade.h"
#include "../../Engine/Physics/CollisionQuery.h"

namespace Game {

void Grenade::Init(const glm::vec3& position, const glm::vec3& velocity, GrenadeKind kind) {
    m_position = position;
    m_velocity = velocity;
    m_kind = kind;
    m_data = GetGrenadeData(kind);
    m_fuseRemaining = m_data.fuseSeconds;
}

bool Grenade::Update(float deltaTime, const std::vector<Engine::Brush>& worldBrushes) {
    m_velocity.y -= kGravity * deltaTime;

    const glm::vec3 start = m_position;
    const glm::vec3 end = start + m_velocity * deltaTime;
    const Engine::TraceResult trace = Engine::TraceMove(start, end, worldBrushes);
    m_position = trace.endPosition;

    if (trace.hit) {
        // ClipVelocity's overbounce parameter removes (1+restitution) times
        // the into-surface component: at exactly 1.0 that's a dead stop
        // along the normal (a slide), so 1.0 + kRestitution reflects
        // outward with kRestitution of the original inward speed — a
        // bounce that loses energy each time, rather than the infinite
        // perfect-elastic bounce a plain reflection formula would give.
        m_velocity = Engine::ClipVelocity(m_velocity, trace.planeNormal, 1.0f + kRestitution);
    }

    m_fuseRemaining -= deltaTime;
    return m_fuseRemaining <= 0.0f;
}

} // namespace Game
