#include "Time.h"

#include <GLFW/glfw3.h>
#include <algorithm>

namespace Engine {

void Time::Init() {
    m_lastFrameTime = glfwGetTime();
    m_deltaTime = 0.0f;
    m_totalTime = 0.0f;
    m_fpsAccumulatedTime = 0.0f;
    m_fpsFrameCount = 0;
    m_fps = 0.0f;
}

void Time::Tick() {
    const double now = glfwGetTime();
    m_deltaTime = static_cast<float>(now - m_lastFrameTime);
    m_lastFrameTime = now;

    // Clamp so a debugger breakpoint or OS hitch doesn't inject a huge dt
    // that would otherwise launch the player through walls next frame.
    m_deltaTime = std::clamp(m_deltaTime, 0.0f, 0.1f);

    m_totalTime += m_deltaTime;

    m_fpsAccumulatedTime += m_deltaTime;
    m_fpsFrameCount += 1;
    if (m_fpsAccumulatedTime >= 0.25f) {
        m_fps = static_cast<float>(m_fpsFrameCount) / m_fpsAccumulatedTime;
        m_fpsAccumulatedTime = 0.0f;
        m_fpsFrameCount = 0;
    }
}

} // namespace Engine
