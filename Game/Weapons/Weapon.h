#pragma once

#include "WeaponData.h"
#include "../Combat/Damageable.h"
#include <glm/glm.hpp>
#include <vector>

namespace Game {

// One ray to hit-test for a fired shot. A normal weapon produces one;
// a shotgun produces WeaponData::pelletCount of them, each independently
// spread. maxDistance is carried per-ray since it differs between a bullet
// (WeaponData::maxRange) and a melee swing (WeaponData::meleeRange).
struct HitscanRay {
    glm::vec3 origin;
    glm::vec3 direction;
    float maxDistance;
};

// Base class covering everything that's the same regardless of which
// weapon this is: ammo, fire-rate cooldown, reload timing, spread derived
// from stats, and recoil accumulation/recovery. A plain Weapon configured
// by WeaponData handles every hitscan weapon — pistols, SMGs, rifles,
// snipers, and machine guns differ only in their numbers, not in behavior,
// so they don't need their own subclasses. Shotgun and Melee override
// TryFire() because they genuinely behave differently: a shotgun fires many
// pellets at once, and melee ignores ammo and uses a short fixed range
// instead of a bullet.
class Weapon {
public:
    void Init(const WeaponData& data);
    virtual ~Weapon() = default;

    // Advances reload/recoil-recovery timers by one frame. Returns the
    // recoil pitch recovered this frame, as a delta to apply to the camera
    // (always <= 0, i.e. "settle the view back down"); 0 if fully settled.
    float Update(float deltaTime);

    // Attempts to fire. Returns the ray(s) to hit-test (empty if the weapon
    // couldn't fire: on cooldown, empty magazine, or mid-reload). On a
    // successful fire, also writes the instantaneous recoil punch to apply
    // to the camera via outRecoilPitchDelta/outRecoilYawDelta (both 0 if it
    // didn't fire).
    virtual std::vector<HitscanRay> TryFire(const glm::vec3& origin, const glm::vec3& aimDirection,
        bool isMoving, bool isCrouching, float& outRecoilPitchDelta, float& outRecoilYawDelta);

    void StartReload();

    // Resupplies reserve ammo back to the weapon's starting amount — the
    // temporary buy-menu stand-in (Milestone 5's economy exists before
    // Milestone 8's real buy UI does) calls this rather than a proper
    // "purchase N rounds" model.
    void RefillReserveAmmo();

    // Range falloff only. Prefer ComputeDamage() below, which also applies
    // the hit-zone multiplier; this is exposed separately so it stays easy
    // to test/tune the falloff curve on its own.
    float ComputeDamageAtDistance(float distance) const;

    // Range falloff + headshot/leg multiplier — what should actually be
    // dealt to whatever ResolveHitscan() says was hit.
    float ComputeDamage(float distance, HitZone zone) const;

    const WeaponData& GetData() const { return m_data; }
    int GetMagazineAmmo() const { return m_magazineAmmo; }
    int GetReserveAmmo() const { return m_reserveAmmo; }
    bool IsReloading() const { return m_reloading; }
    float GetReloadProgress01() const;

protected:
    float ComputeSpreadDegrees(bool isMoving, bool isCrouching) const;
    glm::vec3 ApplySpread(const glm::vec3& direction, float spreadDegrees) const;
    bool CanFire() const;
    void ConsumeAmmoAndCooldown();
    void ApplyRecoilImpulse(float& outPitchDelta, float& outYawDelta);

    WeaponData m_data;
    int m_magazineAmmo = 0;
    int m_reserveAmmo = 0;
    float m_timeSinceLastShot = 1000.0f;
    float m_cooldownRemaining = 0.0f;
    bool m_reloading = false;
    float m_reloadTimeRemaining = 0.0f;
    float m_accumulatedRecoilPitch = 0.0f;
};

// Fires WeaponData::pelletCount independent rays in one shot, each with its
// own random spread sample, and consumes only one shell from the magazine.
class ShotgunWeapon : public Weapon {
public:
    std::vector<HitscanRay> TryFire(const glm::vec3& origin, const glm::vec3& aimDirection,
        bool isMoving, bool isCrouching, float& outRecoilPitchDelta, float& outRecoilYawDelta) override;
};

// No ammo, no magazine, no spread, no recoil — a single fixed-range ray
// directly along the aim direction, gated only by the fire-rate cooldown.
class MeleeWeapon : public Weapon {
public:
    std::vector<HitscanRay> TryFire(const glm::vec3& origin, const glm::vec3& aimDirection,
        bool isMoving, bool isCrouching, float& outRecoilPitchDelta, float& outRecoilYawDelta) override;
};

} // namespace Game
