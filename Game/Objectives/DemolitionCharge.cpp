#include "DemolitionCharge.h"

namespace Game {

void DemolitionCharge::Reset() {
    m_state = ChargeState::Carried;
    m_position = glm::vec3(0.0f);
    m_siteName.clear();
    m_fuseRemaining = kFuseDurationSeconds;
    m_defuseElapsed = 0.0f;
    m_beingDefused = false;
}

void DemolitionCharge::Plant(const glm::vec3& position, const std::string& siteName) {
    m_state = ChargeState::Planted;
    m_position = position;
    m_siteName = siteName;
    m_fuseRemaining = kFuseDurationSeconds;
    m_defuseElapsed = 0.0f;
    m_beingDefused = false;
}

void DemolitionCharge::SetDefusing(bool isDefusing) {
    if (!isDefusing) {
        m_defuseElapsed = 0.0f; // stepping away/releasing aborts progress, not just pauses it
    }
    m_beingDefused = isDefusing;
}

bool DemolitionCharge::Update(float deltaTime) {
    if (m_state != ChargeState::Planted) return false;

    if (m_beingDefused) {
        m_defuseElapsed += deltaTime;
        if (m_defuseElapsed >= kDefuseDurationSeconds) {
            m_state = ChargeState::Defused;
            return true;
        }
    }

    m_fuseRemaining -= deltaTime;
    if (m_fuseRemaining <= 0.0f) {
        m_fuseRemaining = 0.0f;
        m_state = ChargeState::Detonated;
        return true;
    }
    return false;
}

float DemolitionCharge::GetDefuseProgress01() const {
    return m_defuseElapsed / kDefuseDurationSeconds;
}

} // namespace Game
