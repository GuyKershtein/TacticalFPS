#include "RoundManager.h"
#include "../Player/PlayerController.h"
#include "../Player/PlayerHealth.h"
#include "../Economy/Wallet.h"
#include "../Bots/Bot.h"
#include "../../Engine/World/Level.h"
#include "../../Engine/World/CollisionWorld.h"

#include <cstdio>
#include <cmath>

namespace Game {

void RoundManager::Init(PlayerController* player, PlayerHealth* playerHealth, Wallet* playerWallet,
    const Engine::Level* level, const Engine::CollisionWorld* collisionWorld) {
    m_player = player;
    m_playerHealth = playerHealth;
    m_playerWallet = playerWallet;
    m_level = level;
    m_collisionWorld = collisionWorld;

    m_playerTeam = TeamId::Assault; // the only populated team until Milestone 6's bots exist
    m_assaultScore = 0;
    m_guardianScore = 0;

    StartBuyPhase();
}

void RoundManager::RegisterGuardianBots(std::vector<Bot*> bots) {
    m_guardianBots = std::move(bots);
}

bool RoundManager::IsTeamEliminated(TeamId team) const {
    bool anyMemberAtAll = false;
    bool anyAlive = false;

    if (m_playerTeam == team) {
        anyMemberAtAll = true;
        if (m_playerHealth && m_playerHealth->IsAlive()) anyAlive = true;
    }
    if (team == TeamId::Guardian) {
        for (const Bot* bot : m_guardianBots) {
            anyMemberAtAll = true;
            if (bot->GetHealth().IsAlive()) anyAlive = true;
        }
    }

    // A team with no members at all hasn't been "eliminated" — it was never
    // populated (e.g. Guardian before Milestone 6's bots are registered).
    return anyMemberAtAll && !anyAlive;
}

const ObjectiveSite* RoundManager::FindSiteInRange(const glm::vec3& position) const {
    if (!m_level) return nullptr;
    for (const auto& entity : m_level->entities) {
        if (auto* site = dynamic_cast<ObjectiveSite*>(entity.get())) {
            if (site->IsWithinRange(position)) return site;
        }
    }
    return nullptr;
}

bool RoundManager::IsWithinDefuseRange(const glm::vec3& position) const {
    if (m_charge.GetState() != ChargeState::Planted) return false;
    const glm::vec3 diff = position - m_charge.GetPosition();
    const float horizontalDistance = std::sqrt(diff.x * diff.x + diff.z * diff.z);
    return horizontalDistance <= kDefuseRadius;
}

float RoundManager::GetPlantProgress01() const {
    return m_plantElapsed / DemolitionCharge::kPlantDurationSeconds;
}

float RoundManager::GetPhaseTimeRemaining() const {
    switch (m_phase) {
        case RoundPhase::Buy: return m_buyTimeRemaining;
        case RoundPhase::Active:
            return m_charge.GetState() == ChargeState::Planted ? m_charge.GetFuseRemaining() : m_roundTimeRemaining;
        case RoundPhase::End: return m_roundEndTimeRemaining;
    }
    return 0.0f;
}

void RoundManager::RespawnPlayerForTeam(TeamId team) {
    if (!m_level || !m_player) return;

    const std::string wantedTeam = (team == TeamId::Assault) ? "assault" : "guardian";
    const std::vector<Engine::SpawnPointEntity*> spawns = m_level->GetSpawnPoints();

    const Engine::SpawnPointEntity* chosen = nullptr;
    for (const Engine::SpawnPointEntity* spawn : spawns) {
        if (spawn->GetTeam() == wantedTeam) {
            chosen = spawn;
            break;
        }
    }
    if (!chosen && !spawns.empty()) {
        std::fprintf(stderr, "[RoundManager] No spawn tagged '%s'; using the first spawn found\n", wantedTeam.c_str());
        chosen = spawns.front();
    }
    if (!chosen) {
        std::fprintf(stderr, "[RoundManager] Map has no spawn points at all\n");
        return;
    }

    m_player->Init(chosen->GetOrigin(), chosen->GetYawDegrees(), m_collisionWorld);
}

void RoundManager::StartBuyPhase() {
    m_phase = RoundPhase::Buy;
    m_buyTimeRemaining = kBuyPhaseDuration;
    m_charge.Reset();
    m_plantElapsed = 0.0f;
    if (m_playerHealth) m_playerHealth->Reset();
    RespawnPlayerForTeam(m_playerTeam);
    for (Bot* bot : m_guardianBots) {
        bot->ResetForRound();
    }
}

void RoundManager::StartActivePhase() {
    m_phase = RoundPhase::Active;
    m_roundTimeRemaining = kRoundDuration;
}

void RoundManager::EndRound(TeamId winner, const std::string& reason) {
    m_phase = RoundPhase::End;
    m_roundEndTimeRemaining = kRoundEndDisplayDuration;

    if (winner == TeamId::Assault) {
        m_assaultScore++;
    } else if (winner == TeamId::Guardian) {
        m_guardianScore++;
    }

    if (m_playerWallet) {
        m_playerWallet->Add(winner == m_playerTeam ? kRoundWinMoney : kRoundLossMoney);
    }

    m_lastRoundResult = std::string(GetTeamName(winner)) + " wins: " + reason;
    std::printf("[RoundManager] %s (Assault %d - %d Guardian)\n", m_lastRoundResult.c_str(), m_assaultScore, m_guardianScore);

    if (m_assaultScore >= kRoundsToWinMatch || m_guardianScore >= kRoundsToWinMatch) {
        std::printf("[RoundManager] MATCH WON by %s! Resetting score (no lobby/menu system yet to end the match properly — Milestone 8).\n",
            m_assaultScore > m_guardianScore ? "Assault" : "Guardian");
        m_assaultScore = 0;
        m_guardianScore = 0;
    }
    std::fflush(stdout);
}

void RoundManager::Update(float deltaTime, const glm::vec3& playerPosition, bool interactKeyHeld) {
    if (m_phase == RoundPhase::Buy) {
        m_buyTimeRemaining -= deltaTime;
        if (m_buyTimeRemaining <= 0.0f) {
            StartActivePhase();
        }
        return;
    }

    if (m_phase == RoundPhase::End) {
        m_roundEndTimeRemaining -= deltaTime;
        if (m_roundEndTimeRemaining <= 0.0f) {
            StartBuyPhase();
        }
        return;
    }

    // RoundPhase::Active from here on.
    if (IsTeamEliminated(TeamId::Assault)) {
        EndRound(TeamId::Guardian, "Assault eliminated");
        return;
    }
    if (IsTeamEliminated(TeamId::Guardian)) {
        EndRound(TeamId::Assault, "Guardian eliminated");
        return;
    }

    if (m_charge.GetState() == ChargeState::Carried) {
        const ObjectiveSite* site = (m_playerTeam == TeamId::Assault) ? FindSiteInRange(playerPosition) : nullptr;
        if (site && interactKeyHeld) {
            m_plantElapsed += deltaTime;
            if (m_plantElapsed >= DemolitionCharge::kPlantDurationSeconds) {
                m_charge.Plant(site->GetOrigin(), site->GetSiteName());
                if (m_playerWallet) m_playerWallet->Add(kPlantBonusMoney);
                m_plantElapsed = 0.0f;
            }
        } else {
            m_plantElapsed = 0.0f;
        }

        m_roundTimeRemaining -= deltaTime;
        if (m_roundTimeRemaining <= 0.0f) {
            EndRound(TeamId::Guardian, "Time expired");
        }
        return;
    }

    if (m_charge.GetState() == ChargeState::Planted) {
        const bool canDefuse = (m_playerTeam == TeamId::Guardian) && IsWithinDefuseRange(playerPosition);
        m_charge.SetDefusing(canDefuse && interactKeyHeld);

        if (m_charge.Update(deltaTime)) {
            if (m_charge.GetState() == ChargeState::Detonated) {
                EndRound(TeamId::Assault, "Charge detonated");
            } else {
                if (m_playerWallet) m_playerWallet->Add(kDefuseBonusMoney);
                EndRound(TeamId::Guardian, "Charge defused");
            }
        }
    }
}

} // namespace Game
