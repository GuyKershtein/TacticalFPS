#include "PlayerHealth.h"

#include <algorithm>

namespace Game {

void PlayerHealth::Reset() {
    m_health = kMaxHealth;
    m_armor = 0.0f;
}

void PlayerHealth::TakeDamage(float amount, HitZone /*zone*/) {
    if (!IsAlive() || m_godMode) return;

    float remaining = amount;
    if (m_armor > 0.0f) {
        const float absorbed = std::min(m_armor, amount * kArmorDamageReduction);
        m_armor -= absorbed;
        remaining -= absorbed;
    }
    m_health = std::max(m_health - remaining, 0.0f);
}

} // namespace Game
