#include "NavWaypointEntity.h"

namespace Engine {

void NavWaypointEntity::OnSpawn() {
    m_origin = GetVector("origin");
}

REGISTER_ENTITY_CLASS("nav_waypoint", NavWaypointEntity);

} // namespace Engine
