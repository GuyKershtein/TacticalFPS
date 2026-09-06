#include "UIRenderer.h"
#include "../Graphics/GLFunctions.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

bool UIRenderer::Init(const std::string& shaderDir) {
    if (!m_shader.LoadFromFiles(shaderDir + "uiquad.vert", shaderDir + "uiquad.frag")) {
        return false;
    }

    const float quad[6][2] = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
    glBindVertexArray(0);

    return true;
}

void UIRenderer::Shutdown() {
    if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    m_shader.Destroy();
}

void UIRenderer::BeginFrame(int screenWidth, int screenHeight) {
    m_projection = glm::ortho(0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.0f);
}

void UIRenderer::DrawRect(float x, float y, float w, float h, const glm::vec3& color, float alpha) {
    m_shader.Use();
    m_shader.SetMat4("uProjection", m_projection);
    m_shader.SetVec2("uOffset", glm::vec2(x, y));
    m_shader.SetVec2("uSize", glm::vec2(w, h));
    m_shader.SetVec3("uColor", color);
    m_shader.SetFloat("uAlpha", alpha);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace Engine
