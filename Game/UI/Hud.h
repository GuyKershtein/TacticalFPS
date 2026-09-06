#pragma once

#include "../Rounds/RoundPhase.h"
#include <glm/glm.hpp>
#include <string>

namespace Engine {
class Font;
class TextRenderer;
class UIRenderer;
} // namespace Engine

namespace Game {

class PlayerHealth;
class Wallet;
class WeaponInventory;
class RoundManager;

// Renders the always-on in-match overlay: crosshair, health/armor, ammo,
// money, round phase/timer, team score, grenade counts. Stateless by
// design — GameApplication owns the shared Font/TextRenderer/UIRenderer and
// all the game-state objects; this just reads them once a frame and draws.
// A plain struct-of-inputs rather than a dozen positional parameters, since
// most of these come straight from GameApplication's own members.
struct HudDrawInfo {
    int screenWidth = 0;
    int screenHeight = 0;
    const PlayerHealth* health = nullptr;
    const Wallet* wallet = nullptr;
    const WeaponInventory* inventory = nullptr;
    const RoundManager* roundManager = nullptr;
    int botsAlive = 0;
    int botsTotal = 0;
    const int* grenadeCounts = nullptr; // 4 entries, indexed by GrenadeKind
    float fps = 0.0f;
};

class Hud {
public:
    static void Render(Engine::TextRenderer& text, Engine::UIRenderer& ui, const Engine::Font& font,
        const HudDrawInfo& info);

private:
    static std::string FormatTime(float seconds);
    static const char* PhaseLabel(RoundPhase phase);
};

} // namespace Game
