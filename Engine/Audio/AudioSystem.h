#pragma once

#include "SoundClip.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

struct ma_engine; // miniaudio.h is kept out of this header so it isn't pulled into every includer

namespace Engine {

// Wraps miniaudio's engine (device I/O + mixing + 3D spatialization) for
// playing our procedurally-generated one-shot sound clips. This is the
// entire "Audio" layer this engine owns; miniaudio itself only provides
// the low-level playback plumbing, same role GLFW plays for windowing.
class AudioSystem {
public:
    // Both declared here but defined in the .cpp (where ActiveSound is a
    // complete type) on purpose. Without this, the compiler implicitly
    // generates both inline wherever AudioSystem is used as a member (i.e.
    // inside GameApplication, instantiated from Main.cpp) — the destructor
    // to clean up std::vector<unique_ptr<ActiveSound>>, and (perhaps less
    // obviously) the default constructor too, which needs the same
    // information to unwind already-constructed members if a later
    // member's constructor were to throw. ActiveSound is only
    // forward-declared below, so neither can be generated at that call site.
    AudioSystem();
    ~AudioSystem();

    bool Init();
    void Shutdown();

    void SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    // Plays `clip` once at `position`, attenuated by distance from the
    // listener. Safe to call the same clip many times concurrently (e.g.
    // two overlapping gunshots) — each call gets its own playback instance.
    void PlaySound3D(const SoundClip& clip, const glm::vec3& position, float volume = 1.0f);

    // Reaps finished one-shot instances. Call once per frame.
    void Update();

private:
    struct ActiveSound;

    ma_engine* m_engine = nullptr;
    std::vector<std::unique_ptr<ActiveSound>> m_activeSounds;
};

} // namespace Engine
