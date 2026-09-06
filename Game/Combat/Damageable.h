#pragma once

namespace Game {

enum class HitZone { Head, Body, Legs };

// Anything a hitscan shot can damage implements this — target dummies now,
// bots and the player in Milestone 6. Keeping this as a tiny interface
// (rather than baking damage handling into TargetDummy directly) means the
// hitscan system doesn't need to know about any specific gameplay object.
class Damageable {
public:
    virtual ~Damageable() = default;
    virtual void TakeDamage(float amount, HitZone zone) = 0;
    virtual bool IsAlive() const = 0;
};

} // namespace Game
