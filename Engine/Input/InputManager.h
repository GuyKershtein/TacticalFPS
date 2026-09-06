#pragma once

#include <unordered_map>

struct GLFWwindow;

namespace Engine {

// Polls keyboard/mouse state each frame and tracks mouse delta for looking.
//
// GLFW key codes are NOT contiguous (e.g. 0-31 are not valid tokens at all;
// printable keys and function keys sit in separate ranges) and glfwGetKey
// raises a GLFW error for anything outside its documented enum. So instead
// of pre-polling every possible code into a fixed array, keys are queried
// lazily by the exact GLFW_KEY_* code the caller asks about, and only that
// code's previous-frame state is cached (needed for edge detection).
class InputManager {
public:
    void Init(GLFWwindow* window);

    // Call once per frame after glfwPollEvents(), before reading any state.
    void Update();

    bool IsKeyDown(int glfwKeyCode) const;
    bool WasKeyPressed(int glfwKeyCode);  // true only on the transition frame

    bool IsMouseButtonDown(int glfwButton) const;
    bool WasMouseButtonPressed(int glfwButton) const; // true only on the transition frame

    float GetMouseDeltaX() const { return m_mouseDeltaX; }
    float GetMouseDeltaY() const { return m_mouseDeltaY; }

    // Raw cursor position in window pixel coordinates (top-left origin) —
    // for menu hit-testing, as opposed to the delta above which is for
    // camera look. Valid regardless of cursor-lock state.
    float GetCursorX() const { return static_cast<float>(m_lastCursorX); }
    float GetCursorY() const { return static_cast<float>(m_lastCursorY); }

    void SetCursorLocked(bool locked);
    bool IsCursorLocked() const { return m_cursorLocked; }

private:
    // Resyncs the cursor reference position whenever the window regains
    // focus (e.g. alt-tabbing back in), so the OS repositioning/warping the
    // cursor during the focus change isn't misread as a huge mouse-look
    // delta on the next frame. Single static instance pointer is enough
    // since this engine only ever has one window/one InputManager.
    static void FocusCallback(GLFWwindow* window, int focused);
    static InputManager* s_instance;

    static constexpr int kMaxMouseButtons = 8; // GLFW_MOUSE_BUTTON_LAST + 1, a valid contiguous range

    GLFWwindow* m_window = nullptr;

    // Previous-frame down-state per key code actually queried, used only by
    // WasKeyPressed. Populated lazily since we don't know in advance which
    // GLFW_KEY_* codes gameplay code will ask about.
    std::unordered_map<int, bool> m_prevKeyState;

    bool m_mouseButtonsDown[kMaxMouseButtons] = {};
    bool m_prevMouseButtonsDown[kMaxMouseButtons] = {};

    double m_lastCursorX = 0.0;
    double m_lastCursorY = 0.0;
    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
    bool m_firstCursorSample = true;

    bool m_cursorLocked = false;
};

} // namespace Engine
