#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "BotDifficulty.h"
#include "SoundEventBus.h"
#include "../Player/PlayerController.h"
#include "../Player/PlayerHealth.h"
#include "../Weapons/Weapon.h"
#include "../Combat/HitscanSystem.h"
#include "../Teams/Team.h"
#include "../Grenades/SmokeCloud.h"
#include "../../Engine/Navigation/NavGraph.h"
#include "../../Engine/Graphics/DebugLineRenderer.h"

namespace Game {

enum class BotState { Patrol, Investigate, Attack, Retreat, Defend };

// A bot: a PlayerController (reused as-is — a bot is just a differently
// driven mover, so it gets the exact same movement/collision/gravity/crouch
// as the human player, not a parallel physics implementation) plus a
// perception + decision layer that drives that controller's inputs instead
// of keyboard/mouse, and a Weapon + PlayerHealth so it's a real combat
// participant that can shoot and be shot through the same systems
// TargetDummy and the human player already use.
//
// Bot plant/defuse is explicitly out of scope for this milestone — see
// Milestone 6's report for why it doesn't fit RoundManager's current
// single-human-actor API without a real refactor. Bots fight and hold
// ground; they don't yet touch the Demolition Charge.
class Bot {
public:
    void Init(const glm::vec3& spawnPosition, float spawnYawDegrees, TeamId team, BotDifficulty difficulty,
        const Engine::CollisionWorld* collisionWorld, const Engine::NavGraph* navGraph, std::unique_ptr<Weapon> weapon);

    // Resets position/health/state for a new round, keeping team/difficulty/
    // weapon as-is — what RoundManager calls at the start of each Buy phase.
    void ResetForRound();

    void Update(float deltaTime, const glm::vec3& playerEyePosition, bool playerAlive, TeamId playerTeam,
        const std::vector<Engine::Brush>& worldBrushes, const SoundEventBus& soundBus,
        const std::vector<HitscanTarget>& playerHitTargets, Engine::DebugLineRenderer* debugLines,
        const std::vector<SmokeCloud>& smokeClouds = {});

    // Called by whatever resolves a flashbang detonation once it's decided
    // this bot has line of sight to the flash — blindDurationSeconds is
    // already scaled by distance/angle by the caller (GameApplication),
    // since that math needs the flash's position/radius, which Bot doesn't
    // otherwise need to know about.
    void ApplyBlind(float blindDurationSeconds);

    const PlayerController& GetController() const { return m_controller; }
    PlayerHealth& GetHealth() { return m_health; }
    const PlayerHealth& GetHealth() const { return m_health; }
    TeamId GetTeam() const { return m_team; }
    BotState GetState() const { return m_state; }
    bool IsBlinded() const { return m_blindRemaining > 0.0f; }

private:
    glm::vec3 GetNextPathPoint(const glm::vec3& destination, float deltaTime);
    void MoveToward(const glm::vec3& target, float deltaTime, bool aimAtTarget);
    void FireIfReady(const glm::vec3& playerEyePosition, const std::vector<Engine::Brush>& worldBrushes,
        const std::vector<HitscanTarget>& playerHitTargets, Engine::DebugLineRenderer* debugLines);

    PlayerController m_controller;
    PlayerHealth m_health;
    std::unique_ptr<Weapon> m_weapon;
    TeamId m_team = TeamId::Guardian;
    BotDifficulty m_difficulty;
    const Engine::NavGraph* m_navGraph = nullptr;

    BotState m_state = BotState::Defend;
    glm::vec3 m_guardPosition{0.0f}; // Defend/Patrol anchor — its spawn point
    float m_spawnYaw = 0.0f;

    glm::vec3 m_lastKnownPlayerPosition{0.0f};
    bool m_hasKnownPlayerPosition = false;
    float m_timeSinceLastSawPlayer = 1000.0f;
    float m_investigateTimer = 0.0f;
    float m_reactionTimer = 0.0f;
    float m_blindRemaining = 0.0f;

    std::vector<int> m_currentPath;
    size_t m_pathIndex = 0;
    float m_repathTimer = 0.0f;
    glm::vec3 m_lastPathDestination{0.0f};
    bool m_hasLastPathDestination = false;
    int m_wanderWaypoint = -1;

    static constexpr float kRetreatHealthThreshold = 30.0f;
    static constexpr float kMemoryDurationSeconds = 5.0f;   // how long a lost target is still worth investigating
    static constexpr float kInvestigateDurationSeconds = 4.0f;
    static constexpr float kAimTurnRateDegreesPerSecond = 180.0f;
    static constexpr float kMoveTurnRateDegreesPerSecond = 220.0f;
    static constexpr float kWaypointArrivalRadius = 48.0f;
    static constexpr float kWanderRadius = 600.0f; // how far from its guard position a Defend bot will roam
    static constexpr float kRepathIntervalSeconds = 1.5f;
};

} // namespace Game
