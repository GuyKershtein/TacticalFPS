#include "GrenadeTypes.h"

namespace Game {

GrenadeData GetGrenadeData(GrenadeKind kind) {
    GrenadeData data;
    switch (kind) {
        case GrenadeKind::Fragmentation:
            data.name = "Fragmentation Charge";
            data.purchaseCost = 300;
            data.fuseSeconds = 2.2f;
            data.maxDamage = 98.0f;
            data.blastRadius = 400.0f;
            break;
        case GrenadeKind::Smoke:
            data.name = "Smoke Canister";
            data.purchaseCost = 300;
            data.fuseSeconds = 1.8f;
            data.smokeRadius = 220.0f;
            data.smokeDurationSeconds = 15.0f;
            break;
        case GrenadeKind::Flashbang:
            data.name = "Flash Charge";
            data.purchaseCost = 200;
            data.fuseSeconds = 1.6f;
            data.flashRadius = 900.0f;
            data.maxBlindDurationSeconds = 4.0f;
            break;
        case GrenadeKind::Decoy:
            data.name = "Decoy Unit";
            data.purchaseCost = 250;
            data.fuseSeconds = 1.0f;
            data.decoyDurationSeconds = 8.0f;
            data.decoyBeepIntervalSeconds = 1.2f;
            break;
    }
    return data;
}

} // namespace Game
