#pragma once

#include <string>

namespace Game {

enum class GrenadeKind { Fragmentation, Smoke, Flashbang, Decoy };

// Every grenade's tuning in one data-driven struct rather than a class per
// kind — GetGrenadeData(kind) is the only thing that varies; the physics
// (Grenade) and the caller's detonation handling are shared.
struct GrenadeData {
    std::string name;
    int purchaseCost = 0;
    float fuseSeconds = 2.0f;
    float throwSpeed = 700.0f;

    float maxDamage = 0.0f;             // Fragmentation
    float blastRadius = 0.0f;           // Fragmentation

    float smokeRadius = 0.0f;           // Smoke
    float smokeDurationSeconds = 0.0f;  // Smoke

    float flashRadius = 0.0f;                 // Flashbang
    float maxBlindDurationSeconds = 0.0f;     // Flashbang

    float decoyDurationSeconds = 0.0f;        // Decoy
    float decoyBeepIntervalSeconds = 0.0f;    // Decoy
};

GrenadeData GetGrenadeData(GrenadeKind kind);

} // namespace Game
