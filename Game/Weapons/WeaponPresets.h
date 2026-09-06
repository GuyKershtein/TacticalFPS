#pragma once

#include "WeaponData.h"

namespace Game {

// A small representative roster proving the data-driven Weapon framework
// handles every category's distinct behavior (single-hitscan, multi-pellet,
// melee) with original names/numbers. Expanding to the full weapon roster
// (Phase 5) from here is pure data entry, not new code.
WeaponData CreateSidearmData();       // pistol
WeaponData CreateCarbineData();       // rifle
WeaponData CreateStreetSweeperData(); // shotgun
WeaponData CreateCombatKnifeData();   // melee

} // namespace Game
