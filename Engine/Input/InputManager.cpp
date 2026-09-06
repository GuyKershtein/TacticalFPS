#include "InputManager.h"

#include <GLFW/glfw3.h>
#include <cstring>

namespace Engine {

InputManager* InputManager::s_instance = nullptr;

void InputManager::FocusCallback(GLFWwindow* /*window*/, int focused) {
    if (focused && s_instance) {
        s_instance->m_firstCursorSample = true;
    }
}

void InputManager::Init(GLFWwindow* window) {
    m_window = window;
    m_prevKeyState.clear();
    std::memset(m_mouseButtonsDown, 0, sizeof(m_mouseButtonsDown));
    std::memset(m_prevMouseButtonsDown, 0, sizeof(m_prevMouseButtonsDown));
    m_firstCursorSample = true;

    s_instance = this;
    glfwSetWindowFocusCallback(window, FocusCallback);
}

void InputManager::Update() {
    std::memcpy(m_prevMouseButtonsDown, m_mouseButtonsDown, sizeof(m_mouseButtonsDown));
    for (int button = 0; button < kMaxMouseButtons; ++button) {
        m_mouseButtonsDown[button] = glfwGetMouseButton(m_window, button) == GLFW_PRESS;
    }

    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    if (m_firstCursorSample) {
        m_lastCursorX = x;
        m_lastCursorY = y;
        m_firstCursorSample = false;
    }
    m_mouseDeltaX = static_cast<float>(x - m_lastCursorX);
    m_mouseDeltaY = static_cast<float>(y - m_lastCursorY);
    m_lastCursorX = x;
    m_lastCursorY = y;
}

bool InputManager::IsKeyDown(int glfwKeyCode) const {
    if (glfwKeyCode == GLFW_KEY_UNKNOWN) return false;
    return glfwGetKey(m_window, glfwKeyCode) == GLFW_PRESS;
}

bool InputManager::WasKeyPressed(int glfwKeyCode) {
    const bool current = IsKeyDown(glfwKeyCode);
    bool& previous = m_prevKeyState[glfwKeyCode]; // inserts false on first query
    const bool pressed = current && !previous;
    previous = current;
    return pressed;
}

bool InputManager::IsMouseButtonDown(int glfwButton) const {
    if (glfwButton < 0 || glfwButton >= kMaxMouseButtons) return false;
    return m_mouseButtonsDown[glfwButton];
}

bool InputManager::WasMouseButtonPressed(int glfwButton) const {
    if (glfwButton < 0 || glfwButton >= kMaxMouseButtons) return false;
    return m_mouseButtonsDown[glfwButton] && !m_prevMouseButtonsDown[glfwButton];
}

void InputManager::SetCursorLocked(bool locked) {
    m_cursorLocked = locked;
    glfwSetInputMode(m_window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // Raw motion deltas come straight from the mouse driver rather than from
    // diffing cursor position, so they aren't affected by GLFW warping the
    // cursor back to center every frame in disabled mode. Without this, the
    // warp-then-recenter can itself register as a huge one-frame delta
    // (most visible the moment the window first gains focus).
    if (locked && glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    // Resync the reference position so unlocking/relocking doesn't produce
    // a huge one-frame mouse delta from the cursor jumping.
    m_firstCursorSample = true;
}

} // namespace Engine
