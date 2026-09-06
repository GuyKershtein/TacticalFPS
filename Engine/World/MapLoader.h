#pragma once

#include <string>
#include "Level.h"

namespace Engine {

// Parses this engine's original text map format (brush + entity blocks, in
// the spirit of the classic id/Valve .map source format) into a Level. See
// Assets/Maps/test_room.tmap for the format by example.
class MapLoader {
public:
    static bool Load(const std::string& path, Level& outLevel);
};

} // namespace Engine
