#include "DebugLineRenderer.h"
#include "GLFunctions.h"
#include "Mesh.h" // reuses Vertex{position,color} so the layout matches basic.vert exactly

#include <algorithm>
#include <cstdio>

namespace Engine {

void DebugLineRenderer::Init() {
    // Uses the unlit shader (not basic.vert/frag): tracers/debug rays have
    // no meaningful surface normal, so they shouldn't be shaded like level
    // geometry — they should always show their exact given color.
    const std::string shaderDir = std::string(PROJECT_ROOT_DIR) + "/Assets/Shaders/";
    if (!m_shader.LoadFromFiles(shaderDir + "unlit.vert", shaderDir + "unlit.frag")) {
        std::fprintf(stderr, "[DebugLineRenderer] Failed to load shader\n");
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    glBindVertexArray(0);
}

void DebugLineRenderer::Shutdown() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_shader.Destroy();
}

void DebugLineRenderer::AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, float durationSeconds) {
    m_lines.push_back({start, end, color, durationSeconds});
}

void DebugLineRenderer::Update(float deltaTime) {
    for (DebugLine& line : m_lines) {
        line.remainingSeconds -= deltaTime;
    }
    m_lines.erase(
        std::remove_if(m_lines.begin(), m_lines.end(), [](const DebugLine& l) { return l.remainingSeconds <= 0.0f; }),
        m_lines.end());
}

void DebugLineRenderer::Render(const glm::mat4& view, const glm::mat4& projection) {
    if (m_lines.empty()) return;

    std::vector<Vertex> vertices;
    vertices.reserve(m_lines.size() * 2);
    for (const DebugLine& line : m_lines) {
        // Vertex is {position, normal, color} — normal is meaningless for a
        // line and unread by unlit.vert, but it must still be supplied
        // explicitly. (A prior version of this code passed only 2 of the 3
        // aggregate members, which silently shifted the color into the
        // normal slot and left every tracer rendering black.)
        vertices.push_back({line.start, glm::vec3(0.0f), line.color});
        vertices.push_back({line.end, glm::vec3(0.0f), line.color});
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_DYNAMIC_DRAW);

    m_shader.Use();
    m_shader.SetMat4("uModel", glm::mat4(1.0f));
    m_shader.SetMat4("uView", view);
    m_shader.SetMat4("uProjection", projection);
    m_shader.SetVec3("uTint", glm::vec3(1.0f));
    m_shader.SetFloat("uAlpha", 1.0f);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
}

} // namespace Engine
