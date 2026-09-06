#pragma once

#include "../Graphics/Shader.h"
#include <glm/glm.hpp>
#include <string>

namespace Engine {

// Draws flat-colored rectangles in screen (pixel, top-left origin) space —
// HUD bars, panels, buttons, crosshair. A single static unit quad reused
// for every rect via per-draw offset/size uniforms, since a HUD only issues
// a few dozen of these per frame; no benefit to instancing yet.
class UIRenderer {
public:
    bool Init(const std::string& shaderDir);
    void Shutdown();

    void BeginFrame(int screenWidth, int screenHeight);
    void DrawRect(float x, float y, float w, float h, const glm::vec3& color, float alpha = 1.0f);

private:
    Shader m_shader;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    glm::mat4 m_projection{1.0f};
};

} // namespace Engine
