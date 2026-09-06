#pragma once

#include "../Combat/Damageable.h"

namespace Game {

// The player's health/armor, kept separate from PlayerController (which is
// purely about movement) since damage/death is a distinct concern that
// bots will need an equivalent of in Milestone 6. Implements Damageable so
// hitscan weapons and, later, bot attacks can damage the player through the
// same interface TargetDummy already uses.
//
// Armor absorbs half of incoming damage until it runs out — a simple,
// well-contained version of the "Armor" economy purchase the design brief
// calls for; a fuller model (helmet, degradation curves) can wait until
// there's a buy menu (Milestone 8) to actually purchase nuance through.
class PlayerHealth : public Damageable {
public:
    void Reset(); // full health, no armor — called on respawn

    void TakeDamage(float amount, HitZone zone) override;
    bool IsAlive() const override { return m_health > 0.0f; }

    float GetHealth() const { return m_health; }
    float GetArmor() const { return m_armor; }
    void SetArmor(float armor) { m_armor = armor; }

    // A networked client displays health/armor straight from the host's
    // snapshot rather than computing them locally (the host is what
    // actually resolves damage) — this is that display-only write path.
    void SetHealthDirect(float health) { m_health = health; }

    // Debug tool (Milestone 9) — persists across Reset() (round respawns)
    // since it's a standing debug toggle, not per-round state.
    void SetGodMode(bool enabled) { m_godMode = enabled; }
    bool IsGodMode() const { return m_godMode; }

    static constexpr float kMaxHealth = 100.0f;
    static constexpr float kMaxArmor = 100.0f;
    static constexpr float kArmorDamageReduction = 0.5f; // while armor > 0, this fraction of damage hits armor instead of health

private:
    float m_health = kMaxHealth;
    float m_armor = 0.0f;
    bool m_godMode = false;
};

} // namespace Game
