#pragma once

namespace Game {

// A pre-match Warmup/Lobby phase is Milestone 8 UI territory (there's no
// menu system yet to host it) — this milestone starts straight into Buy.
enum class RoundPhase { Buy, Active, End };

} // namespace Game
