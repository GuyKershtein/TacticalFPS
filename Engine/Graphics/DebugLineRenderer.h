#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Shader.h"

namespace Engine {

// A minimal dynamic line-drawing utility: bullet tracers now, ray/collision/
// navigation visualization later (Phase 20's debugging tools reuse this
// rather than reinventing it). Not for permanent world geometry — that's
// Mesh's job; this rebuilds its vertex buffer every frame since its content
// changes every frame.
class DebugLineRenderer {
public:
    void Init();
    void Shutdown();

    void AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, float durationSeconds);
    void Update(float deltaTime);
    void Render(const glm::mat4& view, const glm::mat4& projection);

private:
    struct DebugLine {
        glm::vec3 start;
        glm::vec3 end;
        glm::vec3 color;
        float remainingSeconds;
    };

    std::vector<DebugLine> m_lines;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    Shader m_shader;
};

} // namespace Engine
