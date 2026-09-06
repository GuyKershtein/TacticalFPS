#pragma once

#include <string>

namespace Game {

enum class WeaponCategory { Pistol, SMG, Shotgun, Rifle, Sniper, MachineGun, Melee };

// Every gameplay-relevant number for a weapon lives here so new weapons are
// authored as data, not code (Development Rules 14/15): Weapon and its two
// behavioral subclasses read these fields, but nothing about a specific
// weapon's identity is hardcoded into the classes themselves.
struct WeaponData {
    std::string name = "Unnamed";
    WeaponCategory category = WeaponCategory::Rifle;
    int purchaseCost = 0; // 0 == always owned, never appears as a buy-menu purchase

    int magazineSize = 30;
    int startingReserveAmmo = 90;
    float fireIntervalSeconds = 0.1f; // time between shots
    bool fullAuto = true;             // true: fires repeatedly while held; false: one shot per click
    float reloadTimeSeconds = 2.5f;

    float damage = 25.0f; // per pellet, for shotguns
    int pelletCount = 1;  // >1 turns one shot into a spread of pellets (shotguns)
    float headshotMultiplier = 4.0f;
    float legMultiplier = 0.75f;

    float effectiveRange = 1500.0f; // full damage up to this distance
    float maxRange = 3000.0f;       // zero damage beyond this distance
    float minDamageMultiplierAtMaxRange = 0.4f;

    float baseSpreadDegrees = 2.0f;
    float movingSpreadMultiplier = 3.0f;         // spread while moving = base * this
    float crouchSpreadMultiplier = 0.6f;         // spread while stationary+crouched = base * this
    float firstShotSpreadMultiplier = 0.2f;      // spread on the first shot after a pause
    float firstShotAccuracyWindowSeconds = 0.3f; // how long since the last shot still counts as "first shot"

    float recoilPitchPerShotDegrees = 1.5f; // instantaneous upward view punch per shot
    float recoilYawPerShotDegrees = 0.5f;   // max random left/right punch per shot (does not auto-recover)
    float recoilRecoverySpeedDegreesPerSecond = 12.0f;

    bool isMelee = false; // ignores ammo/magazine entirely; Fire() uses meleeRange instead of a bullet
    float meleeRange = 64.0f;
};

} // namespace Game
