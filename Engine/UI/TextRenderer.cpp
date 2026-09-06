#include "TextRenderer.h"
#include "../Graphics/GLFunctions.h"

#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace Engine {

bool TextRenderer::Init(const std::string& shaderDir) {
    if (!m_shader.LoadFromFiles(shaderDir + "text.vert", shaderDir + "text.frag")) {
        return false;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // Uploaded fresh per DrawText call below; just reserve the attribute
    // layout here (vec2 position + vec2 uv, interleaved).
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);

    return true;
}

void TextRenderer::Shutdown() {
    if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    m_shader.Destroy();
}

void TextRenderer::BeginFrame(int screenWidth, int screenHeight) {
    // Top-left origin, y grows downward — matches GLFW cursor coordinates
    // and Window::GetWidth/Height, so HUD layout code never has to flip axes.
    m_projection = glm::ortho(0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.0f);
}

float TextRenderer::Draw(const Font& font, const std::string& text, float x, float y,
    const glm::vec3& color, float alpha) {
    if (!font.IsLoaded() || text.empty()) return 0.0f;

    std::vector<float> vertices;
    vertices.reserve(text.size() * 6 * 4);

    float penX = x;
    float penY = y + font.GetPixelHeight(); // baked quads are relative to the baseline

    for (const char c : text) {
        GlyphQuad quad;
        font.GetGlyphQuad(c, penX, penY, quad);
        if (c == ' ') continue;

        // Two triangles, matching the winding the rest of the engine uses
        // (doesn't matter for an unculled 2D overlay pass, but consistent).
        const float verts[6][4] = {
            {quad.x0, quad.y0, quad.s0, quad.t0},
            {quad.x1, quad.y0, quad.s1, quad.t0},
            {quad.x1, quad.y1, quad.s1, quad.t1},
            {quad.x0, quad.y0, quad.s0, quad.t0},
            {quad.x1, quad.y1, quad.s1, quad.t1},
            {quad.x0, quad.y1, quad.s0, quad.t1},
        };
        for (const auto& v : verts) {
            vertices.push_back(v[0]);
            vertices.push_back(v[1]);
            vertices.push_back(v[2]);
            vertices.push_back(v[3]);
        }
    }

    if (vertices.empty()) return penX - x;

    m_shader.Use();
    m_shader.SetMat4("uProjection", m_projection);
    m_shader.SetVec3("uColor", color);
    m_shader.SetFloat("uAlpha", alpha);
    m_shader.SetInt("uAtlas", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.GetTextureId());

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));
    glBindVertexArray(0);

    return penX - x;
}

} // namespace Engine
