#pragma once

#include "../../Engine/World/Entity.h"

namespace Game {

// A location where Assault can plant the Demolition Charge. Map classname
// "objective_site"; "name" (e.g. "Alpha") labels it for the HUD/callouts,
// "radius" defines how close the planter must be.
class ObjectiveSite : public Engine::Entity {
public:
    void OnSpawn() override;

    const glm::vec3& GetOrigin() const { return m_origin; }
    float GetRadius() const { return m_radius; }
    const std::string& GetSiteName() const { return m_siteName; }

    bool IsWithinRange(const glm::vec3& position) const;

private:
    glm::vec3 m_origin{0.0f};
    float m_radius = 100.0f;
    std::string m_siteName = "Alpha";
};

} // namespace Game
