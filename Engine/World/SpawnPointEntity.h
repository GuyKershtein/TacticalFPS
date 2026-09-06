#pragma once

#include "Entity.h"

namespace Engine {

// A location + facing angle the round/spawn system can place a player at.
// Map classname: "info_player_start". A "team" key/value (default "any")
// lets later phases reuse this one class for team-specific spawns instead
// of needing a separate classname per team.
class SpawnPointEntity : public Entity {
public:
    void OnSpawn() override;

    const glm::vec3& GetOrigin() const { return m_origin; }
    float GetYawDegrees() const { return m_yawDegrees; }
    const std::string& GetTeam() const { return m_team; }

private:
    glm::vec3 m_origin{0.0f};
    float m_yawDegrees = 0.0f;
    std::string m_team;
};

} // namespace Engine
