#pragma once

#include "SurfaceMaterial.h"
#include <unordered_map>
#include <string>

namespace Engine {

// Loads named surface materials from a data file (Development Rules 13/14:
// configuration values — especially ones content creators keep extending —
// stay out of code). Brushes reference materials by name only; this is
// where that name gets resolved to actual properties, replacing what used
// to be a hardcoded name->color switch statement inside Brush.cpp.
class MaterialDatabase {
public:
    bool LoadFromFile(const std::string& path);

    // Never fails to return something: an unrecognized name logs a warning
    // and returns the magenta fallback material, the same "unmistakably
    // wrong" signal the old hardcoded version used.
    const SurfaceMaterial& Get(const std::string& name) const;

private:
    std::unordered_map<std::string, SurfaceMaterial> m_materials;
    SurfaceMaterial m_fallback;
};

} // namespace Engine
