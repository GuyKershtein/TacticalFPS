#include "SpawnPointEntity.h"

namespace Engine {

void SpawnPointEntity::OnSpawn() {
    m_origin = GetVector("origin");
    m_yawDegrees = GetFloat("angle", 0.0f);
    m_team = GetString("team", "any");
}

REGISTER_ENTITY_CLASS("info_player_start", SpawnPointEntity);

} // namespace Engine
