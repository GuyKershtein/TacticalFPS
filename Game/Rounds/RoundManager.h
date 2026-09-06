#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "RoundPhase.h"
#include "../Teams/Team.h"
#include "../Objectives/DemolitionCharge.h"
#include "../Objectives/ObjectiveSite.h"

namespace Engine {
class Level;
class CollisionWorld;
} // namespace Engine

namespace Game {

class PlayerController;
class PlayerHealth;
class Wallet;
class Bot;

// Drives the round-based match loop: Buy -> Active -> End -> next round's
// Buy, tracking team scores and running the plant/defuse objective through
// DemolitionCharge.
//
// The player is always Assault for now — Guardian exists fully in the data
// model (team-tagged spawns, defuse eligibility, elimination checks) so
// Milestone 6's bots can occupy it without this system changing shape; it
// just has no members to assign to it yet.
class RoundManager {
public:
    void Init(PlayerController* player, PlayerHealth* playerHealth, Wallet* playerWallet,
        const Engine::Level* level, const Engine::CollisionWorld* collisionWorld);

    // Guardian bots this round loop should respawn between rounds and check
    // for the "team eliminated" win condition — see Milestone 6. Assault
    // has no bots yet (the human always represents it), so there's no
    // equivalent registration for that side.
    void RegisterGuardianBots(std::vector<Bot*> bots);

    // interactKeyHeld: whether the player is holding the interact key this
    // frame. Range/team eligibility for planting vs. defusing is resolved
    // internally (FindSiteInRange / IsWithinDefuseRange), matching how a
    // real player only ever expresses "I'm pressing E" — not which action
    // that should mean.
    void Update(float deltaTime, const glm::vec3& playerPosition, bool interactKeyHeld);

    RoundPhase GetPhase() const { return m_phase; }
    float GetPhaseTimeRemaining() const;
    int GetAssaultScore() const { return m_assaultScore; }
    int GetGuardianScore() const { return m_guardianScore; }
    TeamId GetPlayerTeam() const { return m_playerTeam; }
    const DemolitionCharge& GetCharge() const { return m_charge; }
    float GetPlantProgress01() const;
    const std::string& GetLastRoundResult() const { return m_lastRoundResult; }

    const ObjectiveSite* FindSiteInRange(const glm::vec3& position) const;
    bool IsWithinDefuseRange(const glm::vec3& position) const;

    // TEMP (test-only): lets verification flip which team the sole human
    // player represents, since defuse can't otherwise be exercised without
    // a Guardian-side bot (Milestone 6). Not part of normal gameplay flow.
    void DebugSetPlayerTeam(TeamId team) { m_playerTeam = team; }

private:
    void StartBuyPhase();
    void StartActivePhase();
    void EndRound(TeamId winner, const std::string& reason);
    void RespawnPlayerForTeam(TeamId team);
    bool IsTeamEliminated(TeamId team) const;

    PlayerController* m_player = nullptr;
    PlayerHealth* m_playerHealth = nullptr;
    Wallet* m_playerWallet = nullptr;
    const Engine::Level* m_level = nullptr;
    const Engine::CollisionWorld* m_collisionWorld = nullptr;
    std::vector<Bot*> m_guardianBots;

    RoundPhase m_phase = RoundPhase::Buy;
    float m_buyTimeRemaining = 0.0f;
    float m_roundTimeRemaining = 0.0f;
    float m_roundEndTimeRemaining = 0.0f;
    float m_plantElapsed = 0.0f;

    DemolitionCharge m_charge;
    TeamId m_playerTeam = TeamId::Assault;
    int m_assaultScore = 0;
    int m_guardianScore = 0;
    std::string m_lastRoundResult;

    static constexpr float kBuyPhaseDuration = 20.0f;
    static constexpr float kRoundDuration = 90.0f;
    static constexpr float kRoundEndDisplayDuration = 5.0f;
    static constexpr float kDefuseRadius = 80.0f;
    static constexpr int kRoundsToWinMatch = 4;
    static constexpr int kRoundWinMoney = 300;
    static constexpr int kRoundLossMoney = 150;
    static constexpr int kPlantBonusMoney = 150;
    static constexpr int kDefuseBonusMoney = 150;
};

} // namespace Game
