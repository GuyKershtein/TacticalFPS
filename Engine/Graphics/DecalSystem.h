#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Shader.h"

namespace Engine {

// Flat, static, surface-oriented marks (bullet holes) that persist for a
// while then disappear. No projected-decal/texture-atlas system exists yet
// — each mark is a small flat quad built from Mesh::CreatePlane and rotated
// to lie flush against the surface normal at the impact point.
class DecalSystem {
public:
    void Init();
    void Shutdown();

    void AddDecal(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& color, float size, float lifetimeSeconds);

    void Update(float deltaTime);
    void Render(const glm::mat4& view, const glm::mat4& projection);

private:
    struct Decal {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        float size;
        float remainingLife;
        float totalLife;
    };

    std::vector<Decal> m_decals;
    Mesh m_quadMesh; // unit (1x1) white plane, normal +Y, tinted per-decal via uTint
    Shader m_shader;

    static constexpr float kSurfaceOffset = 0.5f; // pushes the decal slightly off the wall to avoid z-fighting
};

} // namespace Engine
