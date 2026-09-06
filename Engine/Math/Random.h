#pragma once

namespace Engine {

// Centralized RNG so gameplay systems (weapon spread today; grenade bounce
// and bot decision-making later) don't each seed and manage their own
// generator.
class Random {
public:
    static float Range(float minInclusive, float maxInclusive);
    static float SignedRange(float maxAbs); // convenience for [-maxAbs, maxAbs]
};

} // namespace Engine
