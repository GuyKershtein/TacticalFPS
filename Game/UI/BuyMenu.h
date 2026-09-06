#pragma once

#include <string>

namespace Engine {
class Font;
class TextRenderer;
class UIRenderer;
class InputManager;
} // namespace Engine

namespace Game {

class Wallet;
class WeaponInventory;
class PlayerHealth;

// The interactive Buy menu (Milestone 8) — replaces the earlier keyboard
// stand-in (raw B/N key handling that lived directly in GameApplication).
// Numbered-hotkey driven (1-8), matching the genre convention this project
// is inspired by, rather than mouse-click rows — shown only during the Buy
// phase. Mutates wallet/inventory/health/grenade counts directly on a
// successful purchase, the same "the system that owns the rule performs the
// mutation" pattern RoundManager already uses for round-end payouts.
class BuyMenu {
public:
    bool IsOpen() const { return m_open; }
    void Open() { m_open = true; }
    void Close() { m_open = false; }

    // grenadeCounts: 4 entries indexed by GrenadeKind, owned by the caller.
    void Update(float deltaTime, Engine::InputManager& input, Wallet& wallet, WeaponInventory& inventory,
        PlayerHealth& health, int grenadeCounts[4]);

    void Render(Engine::TextRenderer& text, Engine::UIRenderer& ui, const Engine::Font& font,
        int screenWidth, int screenHeight, const Wallet& wallet, const WeaponInventory& inventory,
        const PlayerHealth& health, const int grenadeCounts[4]) const;

private:
    bool m_open = false;
    // Set for a brief moment after a purchase attempt, to flash feedback
    // (green for success, red for "can't afford it") next to the money
    // readout rather than requiring the player to watch the console.
    std::string m_lastMessage;
    float m_messageTimer = 0.0f;
    bool m_lastMessageWasError = false;
};

} // namespace Game
