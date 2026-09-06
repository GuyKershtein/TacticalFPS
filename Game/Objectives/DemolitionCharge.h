#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Game {

enum class ChargeState { Carried, Planted, Detonated, Defused };

// The plantable objective device (this engine's original take on the
// classic "plant/defuse" objective mode — deliberately not using
// Counter-Strike's "C4"/"bomb" terminology). One charge exists per round;
// RoundManager owns the instance and resets it between rounds.
class DemolitionCharge {
public:
    void Reset(); // back to Carried, ready for a new round

    void Plant(const glm::vec3& position, const std::string& siteName);

    // Call every frame with whether a Guardian is actively holding the
    // defuse interaction right now; Update() only advances defuse progress
    // on frames where this was true, and resets progress on the frame it
    // stops (moving away or releasing the key aborts a partial defuse, the
    // standard tactical-shooter convention).
    void SetDefusing(bool isDefusing);

    // Advances fuse/defuse timers. Returns true the frame the state changes
    // to Detonated or Defused, so RoundManager can react exactly once.
    bool Update(float deltaTime);

    ChargeState GetState() const { return m_state; }
    const glm::vec3& GetPosition() const { return m_position; }
    const std::string& GetSiteName() const { return m_siteName; }
    float GetFuseRemaining() const { return m_fuseRemaining; }
    float GetDefuseProgress01() const;

    static constexpr float kFuseDurationSeconds = 30.0f;
    static constexpr float kDefuseDurationSeconds = 5.0f;
    static constexpr float kPlantDurationSeconds = 3.0f;

private:
    ChargeState m_state = ChargeState::Carried;
    glm::vec3 m_position{0.0f};
    std::string m_siteName;
    float m_fuseRemaining = kFuseDurationSeconds;
    float m_defuseElapsed = 0.0f;
    bool m_beingDefused = false;
};

} // namespace Game
