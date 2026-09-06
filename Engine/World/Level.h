#pragma once

#include <vector>
#include <memory>
#include "Brush.h"
#include "Entity.h"
#include "SpawnPointEntity.h"

namespace Engine {

// The parsed, in-memory result of loading one map file: world geometry
// brushes plus every entity placed in the map. Gameplay systems (spawning,
// rounds, bots) query this instead of touching the map parser directly.
class Level {
public:
    std::vector<Brush> worldBrushes;
    std::vector<std::unique_ptr<Entity>> entities;

    std::vector<SpawnPointEntity*> GetSpawnPoints() const;
};

} // namespace Engine
