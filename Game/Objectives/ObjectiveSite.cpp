#include "ObjectiveSite.h"

#include <cmath>

namespace Game {

void ObjectiveSite::OnSpawn() {
    m_origin = GetVector("origin");
    m_radius = GetFloat("radius", 100.0f);
    m_siteName = GetString("name", "Alpha");
}

bool ObjectiveSite::IsWithinRange(const glm::vec3& position) const {
    // Horizontal (XZ) distance only — a planter standing at a slightly
    // different height (e.g. a low step within the site) shouldn't matter.
    const float dx = position.x - m_origin.x;
    const float dz = position.z - m_origin.z;
    return std::sqrt(dx * dx + dz * dz) <= m_radius;
}

REGISTER_ENTITY_CLASS("objective_site", ObjectiveSite);

} // namespace Game
