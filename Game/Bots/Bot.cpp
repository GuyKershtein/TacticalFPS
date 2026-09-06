#include "Bot.h"
#include "BotPerception.h"
#include "../../Engine/Math/Random.h"

#include <cmath>
#include <algorithm>

namespace Game {

namespace {
float HorizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}
} // namespace

void Bot::Init(const glm::vec3& spawnPosition, float spawnYawDegrees, TeamId team, BotDifficulty difficulty,
    const Engine::CollisionWorld* collisionWorld, const Engine::NavGraph* navGraph, std::unique_ptr<Weapon> weapon) {
    m_controller.Init(spawnPosition, spawnYawDegrees, collisionWorld);
    m_health.Reset();
    m_team = team;
    m_difficulty = difficulty;
    m_navGraph = navGraph;
    m_weapon = std::move(weapon);
    m_guardPosition = spawnPosition;
    m_spawnYaw = spawnYawDegrees;
    m_state = BotState::Defend;
    m_currentPath.clear();
    m_pathIndex = 0;
    m_repathTimer = 0.0f;
    m_hasLastPathDestination = false;
    m_wanderWaypoint = -1;
    m_hasKnownPlayerPosition = false;
    m_timeSinceLastSawPlayer = 1000.0f;
    m_reactionTimer = 0.0f;
    m_blindRemaining = 0.0f;
}

void Bot::ResetForRound() {
    m_controller.Respawn(m_guardPosition, m_spawnYaw);
    m_health.Reset();
    m_state = BotState::Defend;
    m_currentPath.clear();
    m_pathIndex = 0;
    m_repathTimer = 0.0f;
    m_hasLastPathDestination = false;
    m_wanderWaypoint = -1;
    m_hasKnownPlayerPosition = false;
    m_timeSinceLastSawPlayer = 1000.0f;
    m_reactionTimer = 0.0f;
    m_blindRemaining = 0.0f;
}

glm::vec3 Bot::GetNextPathPoint(const glm::vec3& destination, float deltaTime) {
    const glm::vec3 currentPos = m_controller.GetFeetPosition();
    m_repathTimer -= deltaTime;

    // A purely time-based repath timer can leave a bot walking a stale
    // route toward wherever it USED to be headed for up to
    // kRepathIntervalSeconds after picking a new destination (e.g. a fresh
    // wander target) — from the bot's perspective that looks like freezing
    // in place once the stale path runs out near the old target. Forcing an
    // immediate repath whenever the destination itself changes fixes that.
    const bool destinationChanged = !m_hasLastPathDestination || HorizontalDistance(destination, m_lastPathDestination) > 1.0f;

    if (m_currentPath.empty() || m_repathTimer <= 0.0f || destinationChanged) {
        if (m_navGraph) {
            m_currentPath = m_navGraph->FindPath(currentPos, destination);
        }
        m_pathIndex = 0;
        m_repathTimer = kRepathIntervalSeconds;
        m_lastPathDestination = destination;
        m_hasLastPathDestination = true;
    }

    if (!m_navGraph || m_currentPath.empty()) {
        return destination; // no nav graph or no path found: walk straight as a fallback
    }

    while (m_pathIndex < m_currentPath.size() &&
        HorizontalDistance(currentPos, m_navGraph->GetWaypointPosition(m_currentPath[m_pathIndex])) < kWaypointArrivalRadius) {
        ++m_pathIndex;
    }

    if (m_pathIndex >= m_currentPath.size()) {
        return destination;
    }
    return m_navGraph->GetWaypointPosition(m_currentPath[m_pathIndex]);
}

void Bot::MoveToward(const glm::vec3& target, float deltaTime, bool aimAtTarget) {
    if (aimAtTarget) {
        m_controller.TurnToward(target, kMoveTurnRateDegreesPerSecond, deltaTime);
    }

    // Walk forward in whatever direction the controller is currently facing
    // — TurnToward converges the facing onto the target over a few frames,
    // and forward-walking-while-turning is enough to arrive without needing
    // a separate strafing/steering model.
    const float distance = HorizontalDistance(m_controller.GetFeetPosition(), target);
    const float wishForward = distance > kWaypointArrivalRadius * 0.5f ? 1.0f : 0.0f;
    m_controller.Update(deltaTime, wishForward, 0.0f, false, false, false);
}

void Bot::FireIfReady(const glm::vec3& playerEyePosition, const std::vector<Engine::Brush>& worldBrushes,
    const std::vector<HitscanTarget>& playerHitTargets, Engine::DebugLineRenderer* debugLines) {
    if (!m_weapon) return;
    if (m_reactionTimer < m_difficulty.reactionTimeSeconds) return;

    const glm::vec3 origin = m_controller.GetEyePosition();
    glm::vec3 aimDirection = glm::normalize(playerEyePosition - origin);

    // Extra difficulty-scaled aim error on top of whatever spread the
    // weapon itself already applies for movement/stance.
    if (m_difficulty.aimSpreadDegrees > 0.0f) {
        const glm::vec3 worldUp = std::abs(aimDirection.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 right = glm::normalize(glm::cross(aimDirection, worldUp));
        const glm::vec3 up = glm::cross(right, aimDirection);
        const float errorRad = glm::radians(Engine::Random::Range(0.0f, m_difficulty.aimSpreadDegrees));
        const float angle = Engine::Random::Range(0.0f, 6.2831853f);
        const float spread = std::tan(errorRad);
        aimDirection = glm::normalize(aimDirection + right * (spread * std::cos(angle)) + up * (spread * std::sin(angle)));
    }

    float recoilPitch = 0.0f;
    float recoilYaw = 0.0f;
    const std::vector<HitscanRay> rays = m_weapon->TryFire(
        origin, aimDirection, m_controller.GetHorizontalSpeed() > 10.0f, m_controller.IsCrouching(), recoilPitch, recoilYaw);
    if (rays.empty()) {
        if (!m_weapon->IsReloading() && m_weapon->GetMagazineAmmo() <= 0) {
            m_weapon->StartReload();
        }
        return;
    }

    for (const HitscanRay& ray : rays) {
        const HitscanResult hit = ResolveHitscan(ray.origin, ray.direction, ray.maxDistance, worldBrushes, playerHitTargets);
        glm::vec3 tracerEnd = ray.origin + ray.direction * ray.maxDistance;
        glm::vec3 tracerColor(0.3f, 0.6f, 1.0f); // blue-ish: distinguishes bot fire from the player's tracers
        if (hit.hit) {
            tracerEnd = hit.point;
            if (hit.hitDamageable) {
                hit.hitDamageable->TakeDamage(m_weapon->ComputeDamage(hit.distance, hit.hitZone), hit.hitZone);
            }
        }
        if (debugLines) debugLines->AddLine(ray.origin, tracerEnd, tracerColor, 0.05f);
    }
}

void Bot::ApplyBlind(float blindDurationSeconds) {
    m_blindRemaining = std::max(m_blindRemaining, blindDurationSeconds);
}

void Bot::Update(float deltaTime, const glm::vec3& playerEyePosition, bool playerAlive, TeamId playerTeam,
    const std::vector<Engine::Brush>& worldBrushes, const SoundEventBus& soundBus,
    const std::vector<HitscanTarget>& playerHitTargets, Engine::DebugLineRenderer* debugLines,
    const std::vector<SmokeCloud>& smokeClouds) {
    if (!m_health.IsAlive()) return;

    m_weapon->Update(deltaTime); // reload/cooldown timers; recoil recovery is irrelevant without a human view-punch

    if (m_blindRemaining > 0.0f) {
        m_blindRemaining -= deltaTime;
    }

    // A same-team player (e.g. a human who's currently Guardian) is never a
    // hostile target — without this, a bot would happily shoot an ally.
    // A blinded bot can't perceive anything regardless of LOS.
    const bool playerIsHostile = playerAlive && playerTeam != m_team && m_blindRemaining <= 0.0f;

    const glm::vec3 eyePosition = m_controller.GetEyePosition();
    const glm::vec3 forward = m_controller.GetCamera().GetForward();
    const bool canSeePlayer = playerIsHostile && BotPerception::CanSee(eyePosition, forward, playerEyePosition, m_difficulty, worldBrushes, smokeClouds);

    if (canSeePlayer) {
        m_lastKnownPlayerPosition = playerEyePosition;
        m_hasKnownPlayerPosition = true;
        m_timeSinceLastSawPlayer = 0.0f;
    } else {
        m_timeSinceLastSawPlayer += deltaTime;
    }

    const bool lowHealth = m_health.GetHealth() <= kRetreatHealthThreshold;

    // --- State transitions ---
    if (canSeePlayer) {
        m_state = lowHealth ? BotState::Retreat : BotState::Attack;
    } else if (m_state == BotState::Attack || m_state == BotState::Retreat) {
        m_reactionTimer = 0.0f;
        if (m_hasKnownPlayerPosition && m_timeSinceLastSawPlayer < kMemoryDurationSeconds) {
            m_state = BotState::Investigate;
            m_investigateTimer = kInvestigateDurationSeconds;
        } else {
            m_state = BotState::Defend;
        }
    } else if (m_state == BotState::Investigate) {
        m_investigateTimer -= deltaTime;
        if (m_investigateTimer <= 0.0f) {
            m_state = BotState::Defend;
        }
    } else {
        // Idle (Defend/Patrol): a nearby noise is worth checking out.
        if (const SoundEvent* sound = BotPerception::FindAudibleSound(eyePosition, soundBus.GetEvents(), m_difficulty)) {
            m_state = BotState::Investigate;
            m_lastKnownPlayerPosition = sound->position;
            m_hasKnownPlayerPosition = true;
            m_investigateTimer = kInvestigateDurationSeconds;
        }
    }

    // --- Act on the current state ---
    switch (m_state) {
        case BotState::Attack: {
            m_reactionTimer += deltaTime;
            m_controller.TurnToward(playerEyePosition, kAimTurnRateDegreesPerSecond, deltaTime);
            m_controller.Update(deltaTime, 0.0f, 0.0f, false, false, false); // hold ground and fight
            FireIfReady(playerEyePosition, worldBrushes, playerHitTargets, debugLines);
            break;
        }
        case BotState::Retreat: {
            m_reactionTimer += deltaTime;
            m_controller.TurnToward(playerEyePosition, kAimTurnRateDegreesPerSecond, deltaTime);
            // Backpedal (negative wishForward) while still facing the
            // player — a "fighting retreat" without needing a separate
            // steering model for moving away while aiming.
            m_controller.Update(deltaTime, -1.0f, 0.0f, false, false, false);
            FireIfReady(playerEyePosition, worldBrushes, playerHitTargets, debugLines);
            break;
        }
        case BotState::Investigate: {
            m_reactionTimer = 0.0f;
            MoveToward(GetNextPathPoint(m_lastKnownPlayerPosition, deltaTime), deltaTime, true);
            break;
        }
        case BotState::Defend:
        case BotState::Patrol: {
            m_reactionTimer = 0.0f;
            const glm::vec3 currentPos = m_controller.GetFeetPosition();
            const bool needsNewWanderPoint = m_wanderWaypoint < 0 ||
                (m_navGraph && HorizontalDistance(currentPos, m_navGraph->GetWaypointPosition(m_wanderWaypoint)) < kWaypointArrivalRadius);

            if (needsNewWanderPoint && m_navGraph && m_navGraph->GetWaypointCount() > 0) {
                // Pick a new nearby wander point so a Defend bot patrols the
                // area around its post instead of standing frozen in place.
                for (int attempt = 0; attempt < 8; ++attempt) {
                    const int candidate = static_cast<int>(Engine::Random::Range(0.0f, static_cast<float>(m_navGraph->GetWaypointCount()) - 0.001f));
                    if (HorizontalDistance(m_guardPosition, m_navGraph->GetWaypointPosition(candidate)) <= kWanderRadius) {
                        m_wanderWaypoint = candidate;
                        break;
                    }
                }
            }

            const glm::vec3 destination = (m_wanderWaypoint >= 0 && m_navGraph) ? m_navGraph->GetWaypointPosition(m_wanderWaypoint) : m_guardPosition;
            MoveToward(GetNextPathPoint(destination, deltaTime), deltaTime, true);
            break;
        }
    }
}

} // namespace Game
