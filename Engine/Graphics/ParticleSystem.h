#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Shader.h"

namespace Engine {

// A small pool of physics-simulated particles (debris/sparks/smoke puffs),
// rendered as tumbling colored cubes. There's no billboard/texture-sprite
// system yet, so a small tinted cube is the simplest visual that still
// reads as "debris" at this project's low-poly fidelity — swapping in real
// billboarded sprites later only touches Render().
class ParticleSystem {
public:
    void Init();
    void Shutdown();

    void SpawnBurst(const glm::vec3& position, const glm::vec3& color, int count,
        float speedMin, float speedMax, float lifetimeSeconds, float size, bool useGravity = true);

    void Update(float deltaTime);
    void Render(const glm::mat4& view, const glm::mat4& projection);

private:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 color;
        float remainingLife;
        float totalLife;
        float size;
        bool useGravity;
    };

    std::vector<Particle> m_particles;
    Mesh m_cubeMesh; // unit white cube, tinted per-particle via the unlit shader's uTint
    Shader m_shader;

    static constexpr float kGravity = 500.0f;
    static constexpr float kDrag = 1.2f;
};

} // namespace Engine
