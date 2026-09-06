#pragma once

#include "Font.h"
#include "../Graphics/Shader.h"
#include <glm/glm.hpp>
#include <string>

namespace Engine {

// Draws baked Font glyph quads as alpha-blended textured triangles in
// screen (pixel, top-left origin) space. One draw call per string — HUD/menu
// text volumes per frame are small enough that per-glyph batching isn't
// worth the complexity yet.
class TextRenderer {
public:
    bool Init(const std::string& shaderDir);
    void Shutdown();

    // Call once per frame before any DrawText calls, with the current
    // window size, so the orthographic projection matches the framebuffer.
    void BeginFrame(int screenWidth, int screenHeight);

    // Draws `text` with its top-left corner at (x, y). Returns the width in
    // pixels, so callers can right-align or center without a separate
    // measure pass.
    float Draw(const Font& font, const std::string& text, float x, float y,
        const glm::vec3& color, float alpha = 1.0f);

private:
    Shader m_shader;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    glm::mat4 m_projection{1.0f};
};

} // namespace Engine
