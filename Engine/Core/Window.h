#pragma once

#include <string>

struct GLFWwindow;

namespace Engine {

// Owns the OS window and the GL context that lives inside it. Nothing in
// here knows about gameplay; it is pure platform plumbing so the rest of the
// engine only ever talks to GLFW through this one class.
class Window {
public:
    bool Create(int width, int height, const std::string& title);
    void Destroy();

    bool ShouldClose() const;
    void SwapBuffers();
    void SetTitle(const std::string& title);

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetAspectRatio() const { return m_height > 0 ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.0f; }

    GLFWwindow* GetHandle() const { return m_window; }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
};

} // namespace Engine
