#include "Weapon.h"
#include "../../Engine/Math/Random.h"

#include <algorithm>
#include <cmath>

namespace Game {

void Weapon::Init(const WeaponData& data) {
    m_data = data;
    m_magazineAmmo = data.magazineSize;
    m_reserveAmmo = data.startingReserveAmmo;
    m_timeSinceLastShot = 1000.0f;
    m_cooldownRemaining = 0.0f;
    m_reloading = false;
    m_reloadTimeRemaining = 0.0f;
    m_accumulatedRecoilPitch = 0.0f;
}

float Weapon::Update(float deltaTime) {
    if (m_cooldownRemaining > 0.0f) {
        m_cooldownRemaining -= deltaTime;
    }
    m_timeSinceLastShot += deltaTime;

    if (m_reloading) {
        m_reloadTimeRemaining -= deltaTime;
        if (m_reloadTimeRemaining <= 0.0f) {
            const int needed = m_data.magazineSize - m_magazineAmmo;
            const int taken = std::min(needed, m_reserveAmmo);
            m_magazineAmmo += taken;
            m_reserveAmmo -= taken;
            m_reloading = false;
        }
    }

    // Recover accumulated (vertical) recoil back toward zero and report the
    // delta so the caller can settle the camera by the same amount. Yaw
    // recoil is a one-shot nudge only — see WeaponData::recoilYawPerShotDegrees.
    if (m_accumulatedRecoilPitch <= 0.0f) {
        return 0.0f;
    }
    const float recoveryStep = m_data.recoilRecoverySpeedDegreesPerSecond * deltaTime;
    const float pitchRecovered = std::min(recoveryStep, m_accumulatedRecoilPitch);
    m_accumulatedRecoilPitch -= pitchRecovered;
    return -pitchRecovered;
}

bool Weapon::CanFire() const {
    if (m_reloading) return false;
    if (m_cooldownRemaining > 0.0f) return false;
    if (!m_data.isMelee && m_magazineAmmo <= 0) return false;
    return true;
}

void Weapon::ConsumeAmmoAndCooldown() {
    if (!m_data.isMelee) {
        m_magazineAmmo -= 1;
    }
    m_cooldownRemaining = m_data.fireIntervalSeconds;
    m_timeSinceLastShot = 0.0f;
}

void Weapon::ApplyRecoilImpulse(float& outPitchDelta, float& outYawDelta) {
    outPitchDelta = m_data.recoilPitchPerShotDegrees;
    outYawDelta = Engine::Random::SignedRange(m_data.recoilYawPerShotDegrees);
    m_accumulatedRecoilPitch += outPitchDelta;
}

float Weapon::ComputeSpreadDegrees(bool isMoving, bool isCrouching) const {
    float spread = m_data.baseSpreadDegrees;
    if (m_timeSinceLastShot >= m_data.firstShotAccuracyWindowSeconds) {
        spread *= m_data.firstShotSpreadMultiplier;
    }
    if (isMoving) {
        spread *= m_data.movingSpreadMultiplier;
    } else if (isCrouching) {
        spread *= m_data.crouchSpreadMultiplier;
    }
    return spread;
}

glm::vec3 Weapon::ApplySpread(const glm::vec3& direction, float spreadDegrees) const {
    if (spreadDegrees <= 0.0f) return direction;

    const glm::vec3 worldUp = std::abs(direction.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(direction, worldUp));
    const glm::vec3 up = glm::cross(right, direction);

    const float spreadRad = glm::radians(spreadDegrees);
    const float angle = Engine::Random::Range(0.0f, 6.2831853f);
    const float radius = Engine::Random::Range(0.0f, std::tan(spreadRad));

    const glm::vec3 offsetDir = direction + right * (radius * std::cos(angle)) + up * (radius * std::sin(angle));
    return glm::normalize(offsetDir);
}

std::vector<HitscanRay> Weapon::TryFire(const glm::vec3& origin, const glm::vec3& aimDirection,
    bool isMoving, bool isCrouching, float& outRecoilPitchDelta, float& outRecoilYawDelta) {
    outRecoilPitchDelta = 0.0f;
    outRecoilYawDelta = 0.0f;
    if (!CanFire()) return {};

    const float spread = ComputeSpreadDegrees(isMoving, isCrouching);
    HitscanRay ray;
    ray.origin = origin;
    ray.direction = ApplySpread(aimDirection, spread);
    ray.maxDistance = m_data.maxRange;

    ConsumeAmmoAndCooldown();
    ApplyRecoilImpulse(outRecoilPitchDelta, outRecoilYawDelta);

    return {ray};
}

void Weapon::StartReload() {
    if (m_reloading) return;
    if (m_data.isMelee) return;
    if (m_magazineAmmo >= m_data.magazineSize) return;
    if (m_reserveAmmo <= 0) return;
    m_reloading = true;
    m_reloadTimeRemaining = m_data.reloadTimeSeconds;
}

void Weapon::RefillReserveAmmo() {
    m_reserveAmmo = m_data.startingReserveAmmo;
}

float Weapon::ComputeDamageAtDistance(float distance) const {
    if (distance <= m_data.effectiveRange) return m_data.damage;
    if (distance >= m_data.maxRange) return m_data.damage * m_data.minDamageMultiplierAtMaxRange;

    const float t = (distance - m_data.effectiveRange) / (m_data.maxRange - m_data.effectiveRange);
    const float multiplier = 1.0f + t * (m_data.minDamageMultiplierAtMaxRange - 1.0f);
    return m_data.damage * multiplier;
}

float Weapon::ComputeDamage(float distance, HitZone zone) const {
    float damage = ComputeDamageAtDistance(distance);
    switch (zone) {
        case HitZone::Head: damage *= m_data.headshotMultiplier; break;
        case HitZone::Legs: damage *= m_data.legMultiplier; break;
        case HitZone::Body: default: break;
    }
    return damage;
}

float Weapon::GetReloadProgress01() const {
    if (!m_reloading || m_data.reloadTimeSeconds <= 0.0f) return 0.0f;
    return 1.0f - (m_reloadTimeRemaining / m_data.reloadTimeSeconds);
}

std::vector<HitscanRay> ShotgunWeapon::TryFire(const glm::vec3& origin, const glm::vec3& aimDirection,
    bool isMoving, bool isCrouching, float& outRecoilPitchDelta, float& outRecoilYawDelta) {
    outRecoilPitchDelta = 0.0f;
    outRecoilYawDelta = 0.0f;
    if (!CanFire()) return {};

    const float spread = ComputeSpreadDegrees(isMoving, isCrouching);
    std::vector<HitscanRay> rays;
    rays.reserve(static_cast<size_t>(m_data.pelletCount));
    for (int i = 0; i < m_data.pelletCount; ++i) {
        HitscanRay ray;
        ray.origin = origin;
        ray.direction = ApplySpread(aimDirection, spread);
        ray.maxDistance = m_data.maxRange;
        rays.push_back(ray);
    }

    ConsumeAmmoAndCooldown();
    ApplyRecoilImpulse(outRecoilPitchDelta, outRecoilYawDelta);

    return rays;
}

std::vector<HitscanRay> MeleeWeapon::TryFire(const glm::vec3& origin, const glm::vec3& aimDirection,
    bool /*isMoving*/, bool /*isCrouching*/, float& outRecoilPitchDelta, float& outRecoilYawDelta) {
    outRecoilPitchDelta = 0.0f;
    outRecoilYawDelta = 0.0f;
    if (!CanFire()) return {};

    HitscanRay ray;
    ray.origin = origin;
    ray.direction = aimDirection; // no spread on a melee swing
    ray.maxDistance = m_data.meleeRange;

    ConsumeAmmoAndCooldown(); // no-op on ammo (isMelee), still applies the fire-rate cooldown

    return {ray};
}

} // namespace Game
