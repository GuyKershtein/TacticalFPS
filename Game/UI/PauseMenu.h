#pragma once

namespace Engine {
class Font;
class TextRenderer;
class UIRenderer;
class InputManager;
} // namespace Engine

namespace Game {

// The pause overlay (Milestone 8) — Escape freezes the simulation (see
// GameApplication::ProcessInput's early-return while paused) and shows
// Resume/Quit. There's no lobby/main-menu-before-a-match yet since there's
// no multiplayer session to join (that's Milestone 10); this is the whole
// "Menus" scope this milestone needs.
class PauseMenu {
public:
    // Returns true the frame the player chooses Quit, so the caller can
    // close the window; GameApplication owns actually closing it since
    // PauseMenu has no reference to the Window.
    bool Update(Engine::InputManager& input, bool& outShouldResume);

    void Render(Engine::TextRenderer& text, Engine::UIRenderer& ui, const Engine::Font& font,
        int screenWidth, int screenHeight) const;
};

} // namespace Game
