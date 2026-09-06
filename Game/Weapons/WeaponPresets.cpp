#include "WeaponPresets.h"

namespace Game {

WeaponData CreateSidearmData() {
    WeaponData data;
    data.name = "Sidearm";
    data.category = WeaponCategory::Pistol;
    data.magazineSize = 12;
    data.startingReserveAmmo = 36;
    data.fireIntervalSeconds = 0.15f;
    data.fullAuto = false; // one shot per click
    data.reloadTimeSeconds = 1.5f;
    data.damage = 28.0f;
    data.headshotMultiplier = 4.0f;
    data.legMultiplier = 0.75f;
    data.effectiveRange = 900.0f;
    data.maxRange = 2200.0f;
    data.minDamageMultiplierAtMaxRange = 0.35f;
    data.baseSpreadDegrees = 1.5f;
    data.movingSpreadMultiplier = 2.5f;
    data.crouchSpreadMultiplier = 0.6f;
    data.firstShotSpreadMultiplier = 0.3f;
    data.recoilPitchPerShotDegrees = 1.0f;
    data.recoilYawPerShotDegrees = 0.3f;
    data.recoilRecoverySpeedDegreesPerSecond = 14.0f;
    return data;
}

WeaponData CreateCarbineData() {
    WeaponData data;
    data.name = "Carbine";
    data.category = WeaponCategory::Rifle;
    data.purchaseCost = 2500;
    data.magazineSize = 30;
    data.startingReserveAmmo = 90;
    data.fireIntervalSeconds = 0.1f; // 600 rpm
    data.fullAuto = true;
    data.reloadTimeSeconds = 2.6f;
    data.damage = 34.0f;
    data.headshotMultiplier = 4.0f;
    data.legMultiplier = 0.75f;
    data.effectiveRange = 1800.0f;
    data.maxRange = 3600.0f;
    data.minDamageMultiplierAtMaxRange = 0.5f;
    data.baseSpreadDegrees = 2.0f;
    data.movingSpreadMultiplier = 3.5f;
    data.crouchSpreadMultiplier = 0.55f;
    data.firstShotSpreadMultiplier = 0.15f;
    data.recoilPitchPerShotDegrees = 1.8f;
    data.recoilYawPerShotDegrees = 0.6f;
    data.recoilRecoverySpeedDegreesPerSecond = 10.0f;
    return data;
}

WeaponData CreateStreetSweeperData() {
    WeaponData data;
    data.name = "Street Sweeper";
    data.category = WeaponCategory::Shotgun;
    data.purchaseCost = 1700;
    data.magazineSize = 8;
    data.startingReserveAmmo = 24;
    data.fireIntervalSeconds = 0.7f;
    data.fullAuto = false; // one shot (all pellets) per click
    data.reloadTimeSeconds = 3.5f;
    data.damage = 13.0f; // per pellet
    data.pelletCount = 8;
    data.headshotMultiplier = 3.0f;
    data.legMultiplier = 0.8f;
    data.effectiveRange = 350.0f;
    data.maxRange = 1000.0f;
    data.minDamageMultiplierAtMaxRange = 0.2f;
    data.baseSpreadDegrees = 6.0f;
    data.movingSpreadMultiplier = 1.8f;
    data.crouchSpreadMultiplier = 0.8f;
    data.firstShotSpreadMultiplier = 1.0f; // pellets always spread; no tight "first shot" bonus
    data.recoilPitchPerShotDegrees = 3.5f;
    data.recoilYawPerShotDegrees = 1.0f;
    data.recoilRecoverySpeedDegreesPerSecond = 9.0f;
    return data;
}

WeaponData CreateCombatKnifeData() {
    WeaponData data;
    data.name = "Combat Knife";
    data.category = WeaponCategory::Melee;
    data.fireIntervalSeconds = 0.45f;
    data.fullAuto = false;
    data.damage = 55.0f;
    data.headshotMultiplier = 2.0f;
    data.legMultiplier = 1.0f;
    data.isMelee = true;
    data.meleeRange = 64.0f;
    return data;
}

} // namespace Game
