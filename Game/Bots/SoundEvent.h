#pragma once

#include <glm/glm.hpp>

namespace Game {

enum class SoundCategory { Gunshot, Footstep };

// A single noise a bot might hear. Pushed by whatever made the noise
// (weapon fire, footsteps while running) and polled by every bot's
// perception each frame — see SoundEventBus.
struct SoundEvent {
    glm::vec3 position{0.0f};
    float radius = 0.0f; // a bot within this distance can hear it
    SoundCategory category = SoundCategory::Footstep;
};

} // namespace Game
