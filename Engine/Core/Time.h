#pragma once

namespace Engine {

// Tracks frame delta time and a smoothed FPS reading.
// Milestone 1 scope: just enough for the game loop and an on-screen/console
// FPS readout. A more general clock (fixed-step accumulator for physics,
// pause/scale support) can be layered on top in a later milestone if needed.
class Time {
public:
    void Init();

    // Call once per frame, as early as possible in the loop.
    void Tick();

    float GetDeltaTime() const { return m_deltaTime; }
    float GetTotalTime() const { return m_totalTime; }
    float GetFPS() const { return m_fps; }

private:
    double m_lastFrameTime = 0.0;
    float m_deltaTime = 0.0f;
    float m_totalTime = 0.0f;

    // FPS is averaged over a short window instead of reported per-frame,
    // since a single frame's instantaneous 1/dt is too noisy to read.
    float m_fpsAccumulatedTime = 0.0f;
    int m_fpsFrameCount = 0;
    float m_fps = 0.0f;
};

} // namespace Engine
