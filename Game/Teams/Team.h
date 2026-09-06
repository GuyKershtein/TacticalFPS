#pragma once

namespace Game {

// Original terminology for the two sides of the objective mode (Development
// Rule: don't copy Counter-Strike's terminology directly). Assault plants
// the Demolition Charge; Guardian defends the Charge Points and can defuse it.
enum class TeamId { None, Assault, Guardian };

inline TeamId OpposingTeam(TeamId team) {
    if (team == TeamId::Assault) return TeamId::Guardian;
    if (team == TeamId::Guardian) return TeamId::Assault;
    return TeamId::None;
}

inline const char* GetTeamName(TeamId team) {
    switch (team) {
        case TeamId::Assault: return "Assault";
        case TeamId::Guardian: return "Guardian";
        default: return "None";
    }
}

} // namespace Game
