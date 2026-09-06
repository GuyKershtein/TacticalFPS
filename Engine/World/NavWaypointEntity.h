#pragma once

#include "Entity.h"

namespace Engine {

// A single node in the bot navigation graph. Map classname "nav_waypoint";
// just a position — edges between waypoints are derived automatically by
// NavGraph::Build() (see its header for why), not authored here.
class NavWaypointEntity : public Entity {
public:
    void OnSpawn() override;
    const glm::vec3& GetOrigin() const { return m_origin; }

private:
    glm::vec3 m_origin{0.0f};
};

} // namespace Engine
