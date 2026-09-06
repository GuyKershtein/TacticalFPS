#pragma once

namespace Game {

// Tuning knobs that scale a bot's skill. Development Rule 14: gameplay
// values are data, not hardcoded per-bot logic branches.
struct BotDifficulty {
    float reactionTimeSeconds = 0.3f;  // delay between spotting a target and opening fire
    float aimSpreadDegrees = 3.0f;     // extra random aim error on top of the weapon's own spread
    float visionRangeUnits = 1500.0f;
    float visionFovDegrees = 100.0f;
    float hearingRadiusMultiplier = 1.0f; // scales incoming SoundEvent radii

    static BotDifficulty Easy() { return {0.6f, 8.0f, 1000.0f, 90.0f, 0.8f}; }
    static BotDifficulty Medium() { return {0.3f, 3.0f, 1500.0f, 100.0f, 1.0f}; }
    static BotDifficulty Hard() { return {0.12f, 1.0f, 2000.0f, 110.0f, 1.2f}; }
};

} // namespace Game
