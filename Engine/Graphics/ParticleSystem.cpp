#include "ParticleSystem.h"
#include "GLFunctions.h"
#include "../Math/Random.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Engine {

void ParticleSystem::Init() {
    m_cubeMesh = Mesh::CreateCube(glm::vec3(1.0f, 1.0f, 1.0f));

    const std::string shaderDir = std::string(PROJECT_ROOT_DIR) + "/Assets/Shaders/";
    if (!m_shader.LoadFromFiles(shaderDir + "unlit.vert", shaderDir + "unlit.frag")) {
        std::fprintf(stderr, "[ParticleSystem] Failed to load shader\n");
    }
}

void ParticleSystem::Shutdown() {
    m_cubeMesh.Destroy();
    m_shader.Destroy();
}

void ParticleSystem::SpawnBurst(const glm::vec3& position, const glm::vec3& color, int count,
    float speedMin, float speedMax, float lifetimeSeconds, float size, bool useGravity) {
    for (int i = 0; i < count; ++i) {
        // Uniform random direction on a sphere.
        const float theta = Random::Range(0.0f, 6.2831853f);
        const float z = Random::Range(-1.0f, 1.0f);
        const float radius = std::sqrt(std::max(0.0f, 1.0f - z * z));
        const glm::vec3 direction(radius * std::cos(theta), z, radius * std::sin(theta));

        Particle particle;
        particle.position = position;
        particle.velocity = direction * Random::Range(speedMin, speedMax);
        particle.color = color;
        particle.totalLife = lifetimeSeconds * Random::Range(0.8f, 1.2f);
        particle.remainingLife = particle.totalLife;
        particle.size = size * Random::Range(0.7f, 1.3f);
        particle.useGravity = useGravity;
        m_particles.push_back(particle);
    }
}

void ParticleSystem::Update(float deltaTime) {
    for (Particle& particle : m_particles) {
        if (particle.useGravity) {
            particle.velocity.y -= kGravity * deltaTime;
        }
        particle.velocity *= std::max(0.0f, 1.0f - kDrag * deltaTime);
        particle.position += particle.velocity * deltaTime;
        particle.remainingLife -= deltaTime;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p) { return p.remainingLife <= 0.0f; }),
        m_particles.end());
}

void ParticleSystem::Render(const glm::mat4& view, const glm::mat4& projection) {
    if (m_particles.empty()) return;

    m_shader.Use();
    m_shader.SetMat4("uView", view);
    m_shader.SetMat4("uProjection", projection);

    for (const Particle& particle : m_particles) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), particle.position);
        model = glm::scale(model, glm::vec3(particle.size));
        m_shader.SetMat4("uModel", model);
        m_shader.SetVec3("uTint", particle.color);
        m_shader.SetFloat("uAlpha", std::clamp(particle.remainingLife / particle.totalLife, 0.0f, 1.0f));
        m_cubeMesh.Draw();
    }
}

} // namespace Engine
