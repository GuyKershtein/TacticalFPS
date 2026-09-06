#include "Level.h"

namespace Engine {

std::vector<SpawnPointEntity*> Level::GetSpawnPoints() const {
    std::vector<SpawnPointEntity*> result;
    for (const auto& entity : entities) {
        if (auto* spawn = dynamic_cast<SpawnPointEntity*>(entity.get())) {
            result.push_back(spawn);
        }
    }
    return result;
}

} // namespace Engine
