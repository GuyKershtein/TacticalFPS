#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "BotDifficulty.h"
#include "SoundEvent.h"
#include "../Grenades/SmokeCloud.h"
#include "../../Engine/World/Brush.h"

namespace Game {

// Pure functions (no state) for "can this bot perceive that" checks, used
// by Bot's state machine. Kept free of Bot's own data so they're easy to
// reason about and test in isolation.
namespace BotPerception {

// True if targetPosition is within vision range, inside the bot's forward
// FOV cone, not blocked by world geometry, and not obscured by smoke.
bool CanSee(const glm::vec3& eyePosition, const glm::vec3& forward, const glm::vec3& targetPosition,
    const BotDifficulty& difficulty, const std::vector<Engine::Brush>& worldBrushes,
    const std::vector<SmokeCloud>& smokeClouds = {});

// Returns the loudest sound event within hearing range of earPosition, or
// nullptr if none. "Loudest" approximated as closest-relative-to-its-own-
// radius, so a quiet nearby footstep can still win over a distant gunshot.
const SoundEvent* FindAudibleSound(const glm::vec3& earPosition, const std::vector<SoundEvent>& events, const BotDifficulty& difficulty);

} // namespace BotPerception

} // namespace Game
