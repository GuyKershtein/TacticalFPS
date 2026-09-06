#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Engine {

// Everything about a named surface that rendering/gameplay systems need.
// Color is a flat placeholder until real texturing exists; footstepSound
// and impactEffect are named fields now so Milestone 7/12 (bullet impacts,
// footstep audio) have somewhere to read from instead of needing another
// pass through every material's definition — they go unused until then.
struct SurfaceMaterial {
    std::string name;
    glm::vec3 color{1.0f, 0.0f, 1.0f}; // unmistakable magenta default: flags an unregistered material
    std::string footstepSound;
    std::string impactEffect;
};

} // namespace Engine
