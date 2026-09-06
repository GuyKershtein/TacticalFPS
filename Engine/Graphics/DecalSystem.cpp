#include "DecalSystem.h"
#include "GLFunctions.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Engine {

void DecalSystem::Init() {
    m_quadMesh = Mesh::CreatePlane(1.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

    const std::string shaderDir = std::string(PROJECT_ROOT_DIR) + "/Assets/Shaders/";
    if (!m_shader.LoadFromFiles(shaderDir + "unlit.vert", shaderDir + "unlit.frag")) {
        std::fprintf(stderr, "[DecalSystem] Failed to load shader\n");
    }
}

void DecalSystem::Shutdown() {
    m_quadMesh.Destroy();
    m_shader.Destroy();
}

void DecalSystem::AddDecal(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& color, float size, float lifetimeSeconds) {
    m_decals.push_back({position + normal * kSurfaceOffset, normal, color, size, lifetimeSeconds, lifetimeSeconds});

    // A large number of decals would slow rendering with no gameplay
    // benefit (Development Rule 11: simple and reliable over unbounded) —
    // cap the pool by evicting the oldest once full.
    constexpr size_t kMaxDecals = 200;
    if (m_decals.size() > kMaxDecals) {
        m_decals.erase(m_decals.begin());
    }
}

void DecalSystem::Update(float deltaTime) {
    for (Decal& decal : m_decals) {
        decal.remainingLife -= deltaTime;
    }
    m_decals.erase(
        std::remove_if(m_decals.begin(), m_decals.end(), [](const Decal& d) { return d.remainingLife <= 0.0f; }),
        m_decals.end());
}

void DecalSystem::Render(const glm::mat4& view, const glm::mat4& projection) {
    if (m_decals.empty()) return;

    m_shader.Use();
    m_shader.SetMat4("uView", view);
    m_shader.SetMat4("uProjection", projection);

    for (const Decal& decal : m_decals) {
        // CreatePlane's quad lies in local X/Z with normal +Y — build a
        // basis mapping local Y to the surface normal (the same
        // tangent-construction trick Brush::BuildGeometry uses).
        const glm::vec3 up = (std::abs(decal.normal.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 tangent = glm::normalize(glm::cross(up, decal.normal));
        const glm::vec3 bitangent = glm::cross(decal.normal, tangent);

        glm::mat4 basis(1.0f);
        basis[0] = glm::vec4(tangent, 0.0f);
        basis[1] = glm::vec4(decal.normal, 0.0f);
        basis[2] = glm::vec4(bitangent, 0.0f);
        basis[3] = glm::vec4(decal.position, 1.0f);

        const glm::mat4 model = glm::scale(basis, glm::vec3(decal.size, 1.0f, decal.size));
        m_shader.SetMat4("uModel", model);
        m_shader.SetVec3("uTint", decal.color);
        m_shader.SetFloat("uAlpha", std::clamp(decal.remainingLife / decal.totalLife, 0.0f, 1.0f) * 0.9f + 0.1f);
        m_quadMesh.Draw();
    }
}

} // namespace Engine
