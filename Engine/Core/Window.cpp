#include "Window.h"

// GLFunctions.h must come first: it pulls in <Windows.h>, which defines
// APIENTRY. glfw3.h only guards its own APIENTRY definition with #ifndef, so
// including it first causes a macro-redefinition warning when Windows.h is
// included afterward.
#include "../Graphics/GLFunctions.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>

namespace Engine {

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->m_width = width;
        self->m_height = height;
        glViewport(0, 0, width, height);
    }
}

bool Window::Create(int width, int height, const std::string& title) {
    glfwSetErrorCallback([](int code, const char* description) {
        std::fprintf(stderr, "[GLFW] Error %d: %s\n", code, description);
    });

    if (!glfwInit()) {
        std::fprintf(stderr, "[Window] glfwInit failed\n");
        return false;
    }

    // Request an OpenGL 3.3 core context: modern enough for shaders/VAOs,
    // widely supported on "modest hardware" as required by the design doc.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::fprintf(stderr, "[Window] glfwCreateWindow failed\n");
        glfwTerminate();
        return false;
    }

    m_width = width;
    m_height = height;

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // vsync on by default; exposed via Settings later

    if (!LoadGLFunctions()) {
        std::fprintf(stderr, "[Window] Failed to load required OpenGL 3.3 functions\n");
        return false;
    }

    glViewport(0, 0, m_width, m_height);
    return true;
}

void Window::Destroy() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(m_window) != 0;
}

void Window::SwapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::SetTitle(const std::string& title) {
    glfwSetWindowTitle(m_window, title.c_str());
}

} // namespace Engine
